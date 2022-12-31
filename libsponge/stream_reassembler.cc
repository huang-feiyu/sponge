#include "stream_reassembler.hh"

StreamReassembler::StreamReassembler(const size_t capacity)
    : _substring(capacity, '\0'), _bitmap(capacity), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
    if (index >= _next_index + _output.remaining_capacity()) {
        return;
    }
    if (eof && index + data.size() <= _next_index + _output.remaining_capacity()) {
        _eof_flag = true;
    }

    // part of data has not been emitted
    if (index + data.size() > _next_index) {
        for (size_t i = (index > _next_index ? index : _next_index);
             i < _next_index + _output.remaining_capacity() && i < index + data.size();
             i++) {
            if (_bitmap.count(i) == 0) {
                if (_substring.capacity() <= i) {
                    _substring.reserve(i * 2);
                }
                _substring[i] = data[i - index];
                _bitmap.insert(i);
                _unassembled_bytes++;
            }
        }
    }
    while (_bitmap.count(_next_index) > 0) {
        _output.write(_substring[_next_index]);
        _bitmap.erase(_next_index);
        _next_index++;
        _unassembled_bytes--;
    }

    // No remain bytes
    if (_eof_flag && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
