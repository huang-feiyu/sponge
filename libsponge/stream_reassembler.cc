#include "stream_reassembler.hh"

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
    if (index >= _next_index + _capacity) {
        return;
    }
    substring elm{index, data.length(), data};  // default: insert all
    if (index + data.length() <= _next_index) {
        goto end;
    } else if (index < _next_index) {
        size_t offset = _next_index - index;
        elm.data.assign(data.begin() + offset, data.end());
        elm.begin = index + offset;
        elm.length = elm.data.length();
    }
    _unassembled_bytes += elm.length;
    _substrings.emplace(elm);

reloop:
    // merge every possible pair
    for (auto itr1 = _substrings.begin(); itr1 != _substrings.end(); itr1++) {
        for (auto itr2 = itr1; itr2 != _substrings.end(); itr2++) {
            if (itr2 == itr1) {
                continue;
            }
            auto merged = merge(*itr1, *itr2);
            if (merged.length != 0) {
                _unassembled_bytes -= itr1->length + itr2->length - merged.length;
                _substrings.erase(itr2);
                _substrings.erase(itr1);
                _substrings.emplace(merged);
                goto reloop;
            }
            break;
        }
    }

    if (!_substrings.empty() && _substrings.begin()->begin == _next_index) {
        size_t write_bytes = _output.write(_substrings.begin()->data);
        _next_index += write_bytes;
        _unassembled_bytes -= write_bytes;
        _substrings.erase(_substrings.begin());
        // TODO: might write part of the substring
    }

end:
    if (eof) {
        _eof_flag = true;
    }
    if (_eof_flag && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
