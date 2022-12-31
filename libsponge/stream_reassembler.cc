#include "stream_reassembler.hh"

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
    // duplicate
    if (index >= _next_index + _capacity) {
        return;
    }
    substring substr{index,                  // begin
                     data.length(),          // length
                     index + data.length(),  // end
                     data};                  // data
    substrings.emplace(substr);
    _unassembled_bytes += data.length();
    merge_substring();

    while (!substrings.empty() && substrings.begin()->begin == _next_index) {
        auto write_bytes = _output.write(substrings.begin()->data);
        if (write_bytes == 0) {
            break;
        }
        printf("Write %s [%lu]\n", substrings.begin()->data.c_str(), write_bytes);
        _next_index += write_bytes;
        _unassembled_bytes -= write_bytes;
        // TODO: Might only write part of data
        substrings.erase(substrings.begin());
    }

    if (eof) {
        _eof_flag = true;
    }
    if (_eof_flag && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }

void StreamReassembler::merge_substring() {
reloop:
    for (auto itr1 = substrings.begin(); itr1 != substrings.end(); itr1++) {
        for (auto itr2 = substrings.begin(); itr2 != substrings.end(); itr2++) {
            if (itr1 == itr2) {
                continue;
            }
            auto x = itr1;
            auto y = itr2;
            if (itr1->begin > itr2->begin) {
                x = itr2;
                y = itr1;
            }
            if (x->end < y->begin) {
                continue;
            } else if (x->end >= y->end) {
                substrings.erase(y);
                _unassembled_bytes -= y->length;
                goto reloop;
            }
            auto overlap = x->end - y->begin;
            substring substr;
            substr.begin = x->begin;
            substr.length = x->length + y->length - overlap;
            substr.end = substr.begin + substr.length;
            substr.data = x->data + y->data.substr(overlap);
            substrings.emplace(substr);
            substrings.erase(x);
            substrings.erase(y);
            _unassembled_bytes -= x->length;
            _unassembled_bytes -= y->length;
            _unassembled_bytes += substr.length;
            goto reloop;
        }
    }

    printf("substrings: \n");
    for (auto substr : substrings) {
        printf("\t\"%s\" [%zu,%zu]\n", substr.data.c_str(), substr.begin, substr.end);
    }
}
