#include "tcp_receiver.hh"

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!_syn_flag) {
        if (seg.header().syn) {
            _syn_flag = true;
            _isn = seg.header().seqno.raw_value();
        } else {
            return;
        }
    }

    auto abs_seqno = unwrap(seg.header().seqno + static_cast<int>(seg.header().syn) /* + 1 for current SYN */,
                            WrappingInt32(_isn),
                            static_cast<uint64_t>(_reassembler.get_next()));
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1 /* - 1 for SYN */, seg.header().fin);
}

std::optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn_flag) {
        auto written = stream_out().bytes_written() + 1;  // + 1 for SYN
        if (stream_out().input_ended()) {
            written++;  // + 1 for FIN
        }
        return wrap(written, WrappingInt32(_isn));
    } else {
        return std::nullopt;
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
