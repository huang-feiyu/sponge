#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <cassert>
#include <random>

//! \param[in] capacity the capacity of the outgoing bye stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{std::random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {
}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    TCPSegment segment;
    if (!_syn_flag) {
        _syn_flag = true;
        // first hand-shaking: syn & seqno
        segment.header().syn = true;
        send_segment(segment);
        return;
    }

    auto window_size = _window_size > 0 ? _window_size : 1;
    auto remain_size = window_size - (_next_seqno - _recv_seqno);
    assert(_next_seqno >= _recv_seqno);
    // send as much as possible to fill up the window
    while (!_fin_flag && (remain_size = window_size - (_next_seqno - _recv_seqno)) > 0) {
        segment.payload() = _stream.read(std::min(TCPConfig::MAX_PAYLOAD_SIZE, remain_size));
        // the last segment (FIN occupies one byte)
        if (_stream.eof() && segment.length_in_sequence_space() < remain_size) {
            segment.header().fin = true;
            _fin_flag = true;
        }
        if (segment.length_in_sequence_space() == 0) {
            return;
        }
        send_segment(segment);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    auto seqno = unwrap(ackno, _isn, _next_seqno);
    if (seqno > _next_seqno) {
        return;  // invalid
    }
    _window_size = window_size;
    if (seqno <= _recv_seqno) {
        // have been received
        return;
    }
    // update meta-data
    _recv_seqno = seqno;

    // remove received waiting segments
    while (!_segments_wait.empty()) {
        auto segment = _segments_wait.front();
        if (unwrap(segment.header().seqno, _isn, _next_seqno) + segment.length_in_sequence_space() <= seqno) {
            // everything has been received
            _segments_wait.pop();
            _bytes_in_flight -= segment.length_in_sequence_space();
        } else {
            break;
        }
    }

    // fill window if possible
    fill_window();

    // (a) set RTO to initial value
    _retransmission_timeout = _initial_retransmission_timeout;
    // (b) restart timer if there is any outstanding data
    if (_segments_wait.empty()) {
        _retransmission_timer_running = false;
    } else {
        assert(_retransmission_timer_running);
        _retransmission_timer = 0;
    }
    // (c) reset consecutive retransmissions
    _consecutive_retransmissions = 0;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_retransmission_timer_running) {
        // no segment need to retransmit
        return;
    }
    _retransmission_timer += ms_since_last_tick;
    if (_retransmission_timer >= _retransmission_timeout && !_segments_wait.empty()) {
        auto segment = _segments_wait.front();
        // (a) retransmit the youngest segment
        _segments_out.emplace(segment);
        // (b) if window size > 0, update meta-data: consecutive retransmissions & double RTO
        if (_window_size > 0 || segment.header().syn) {
            _consecutive_retransmissions++;
            _retransmission_timeout *= 2;
        }
        // (c) reset timer
        _retransmission_timer = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.emplace(segment);
}

void TCPSender::send_segment(TCPSegment &segment) {
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.emplace(segment);
    _segments_wait.emplace(segment);
    _bytes_in_flight += segment.length_in_sequence_space();
    _next_seqno += segment.length_in_sequence_space();
    if (!_retransmission_timer_running) {
        _retransmission_timer_running = true;
        _retransmission_timer = 0;
    }
}
