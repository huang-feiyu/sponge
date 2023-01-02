#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

//! \param[in] capacity the capacity of the outgoing bye stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{std::random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if (_fin_flag) {
        return;
    }
    if (!_syn_flag) {
        // first hand-shaking: syn & seqno
        TCPSegment segment;
        segment.header().syn = true;
        send_segment(segment);
        return;
    }

    auto window_size = _window_size > 0 ? _window_size : 1;
    // send as much as possible to fill up the window
    while (!_stream.buffer_empty() && window_size - (_next_seqno - _recv_seqno) > 0) {
        auto size = std::min(TCPConfig::MAX_PAYLOAD_SIZE, window_size - (_next_seqno - _recv_seqno));
        TCPSegment segment;
        segment.payload() = _stream.read(size);
        // the last segment
        if (_stream.eof()) {
            segment.header().fin = true;
            _fin_flag = true;
        }
        send_segment(segment);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t seqno = unwrap(ackno, _isn, _next_seqno);
    if (seqno <= _recv_seqno) {
        // have been received
        return;
    }
    // update meta-data
    _recv_seqno = seqno;
    _window_size = window_size;

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
    if (!_segments_wait.empty()) {
        _retransmission_timer_running = false;
        _retransmission_timer = 0;
    }
    // (c) reset consecutive retransmissions
    _consecutive_retransmissions = 0;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {}

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
        _retransmission_timer = true;
        _retransmission_timer = 0;
    }
}
