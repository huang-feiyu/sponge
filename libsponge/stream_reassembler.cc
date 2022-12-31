#include "stream_reassembler.hh"

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    printf("\n=====init %zu=====\n", capacity);
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
    printf("\npush: %s [%zu,%lu) %s\n", data.c_str(), index, index + data.size(), eof ? "eof" : "");
    if (index >= _next_index + _capacity) {
        return;
    }
    substring elm;
    if (index + data.length() <= _next_index) {
        goto end;
    } else if (index < _next_index) {
        size_t offset = _next_index - index;
        elm.data.assign(data.begin() + offset, data.end());
        elm.begin = index + offset;
        elm.length = elm.data.length();
    } else {
        elm.begin = index;
        elm.length = data.length();
        elm.data = data;
    }
    _unassembled_bytes += elm.length;

    // merge substring
    do {
        // merge next
        long merged_bytes = 0;
        auto iter = _substrings.lower_bound(elm);
        while (iter != _substrings.end() && (merged_bytes = merge(elm, *iter)) >= 0) {
            _unassembled_bytes -= merged_bytes;
            _substrings.erase(iter);
            iter = _substrings.lower_bound(elm);
        }
        // merge prev
        if (iter == _substrings.begin()) {
            break;
        }
        iter--;
        while ((merged_bytes = merge(elm, *iter)) >= 0) {
            _unassembled_bytes -= merged_bytes;
            _substrings.erase(iter);
            iter = _substrings.lower_bound(elm);
            if (iter == _substrings.begin()) {
                break;
            }
            iter--;
        }
    } while (false);
    _substrings.emplace(elm);

    printf("substrings:\n");
    for (auto substr : _substrings) {
        printf("\t%s [%zu,%lu)\n", substr.data.c_str(), substr.begin, substr.begin + substr.length);
    }

    if (!_substrings.empty() && _substrings.begin()->begin == _next_index) {
        size_t write_bytes = _output.write(_substrings.begin()->data);
        _next_index += write_bytes;
        _unassembled_bytes -= write_bytes;
        _substrings.erase(_substrings.begin());
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
