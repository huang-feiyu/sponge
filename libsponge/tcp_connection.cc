#include "tcp_connection.hh"

#include <iostream>

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) { DUMMY_CODE(seg); }

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const std::string &data) {
    auto res = _sender.stream_in().write(data);
    send_segment(); // send new data if possible
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

void TCPConnection::end_input_stream() {
    _sender.stream_in().input_ended();
    _sender.fill_window();  // in order to send FIN as early as possible
    send_segment();
}

void TCPConnection::connect() {
    // send a SYN segment
    _sender.fill_window();
    send_segment();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            std::cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
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
        if (!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
            _active = false;
        }
    }
}
