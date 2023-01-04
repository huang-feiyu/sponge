#include "tcp_connection.hh"

#include <iostream>

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!_active) {
        return;
    }
    _time_since_last_segment_received = 0;  // reset timer

    if (seg.header().rst) {
        // connection is done => unclean shutdown
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        return;
    }

    /*===== State machine =====*/
    // ***server***, passive open: CLOSED => LISTEN (not included)
    // LISTEN (***server*** receive handshake#1 [SYN] from ***client***) => SYN RECEIVED
    if (_sender.next_seqno_absolute() == 0) {
        if (seg.header().syn) {
            _receiver.segment_received(seg);
            connect();  // send handshake#2 [SYN/ACK]
        }
    }
    // SYN SENT
    else if (!_receiver.ackno().has_value() && _sender.next_seqno_absolute() == _sender.bytes_in_flight()) {
        // SYN SENT (***client*** receive handshake#2 [SYN/ACK] from ***server***) => ESTABLISHED
        if (seg.header().syn && seg.header().ack) {
            _sender.ack_received(seg.header().ackno, seg.header().win);
            _receiver.segment_received(seg);
            _sender.send_empty_segment();  // send handshake#3 [ACK]
            send_segment();
        }
        // SYN SENT (Exception: simultaneous open, receive SYN) => SYN RECEIVED
        else if (seg.header().syn && !seg.header().ack) {
            _receiver.segment_received(seg);
            _sender.send_empty_segment();  // send handshake#3-like [ACK]
            send_segment();
        }
    }
    // SYN RECEIVED (***server*** receive handshake#3 [ACK] from ***client***) => ESTABLISHED
    else if (_receiver.ackno().has_value() && _sender.next_seqno_absolute() == _sender.bytes_in_flight() &&
             !_receiver.stream_out().input_ended()) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        _receiver.segment_received(seg);
    }
    // ESTABLISHED (normal data transfer)
    else if (_sender.next_seqno_absolute() > _sender.bytes_in_flight() && !_sender.stream_in().eof()) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        _receiver.segment_received(seg);
        if (seg.length_in_sequence_space() > 0) {
            _sender.send_empty_segment();
        }
        _sender.fill_window();
        send_segment();
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const std::string &data) {
    auto res = _sender.stream_in().write(data);
    send_segment();  // send new data if possible
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!_active) {
        return;
    }
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);

    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        // send RST segment => unclean shutdown
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
    }

    send_segment();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().input_ended();
    _sender.fill_window();  // in order to send FIN as early as possible
    send_segment();
}

void TCPConnection::connect() {
    // send a SYN segment ([initiative] handshake#1 or handshake#2)
    // if handshake#1, ***client*** active open: CLOSED => SYN SENT
    // if handshake#2, LISTEN => SYN RECEIVED
    _sender.fill_window();
    send_segment();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            std::cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // send RST segment => unclean shutdown
            TCPSegment seg;
            seg.header().rst = true;
            _segments_out.push(seg);
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            _active = false;
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::send_segment() {
    while (!_sender.segments_out().empty()) {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        // set ACK, ackno, windows_size if possible (i.e. has received ACK from remote peer)
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.emplace(seg);
    }

    // clean shutdown
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // If the inbound stream ends before the TCPConnection has reached EOF on its outbound stream,
        // this variable needs to be set to false.
        _linger_after_streams_finish = false;
    }
    //         preReq #2                         preReq #3                                preReq #1
    if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
        // shutdown if necessary
        if (!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
            _active = false;
        }
    }
}
