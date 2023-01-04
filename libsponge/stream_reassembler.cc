#include "stream_reassembler.hh"

#include <cassert>

StreamReassembler::StreamReassembler(const size_t capacity)
    : _unassemble_strs()
    , _next_assembled_idx(0)
    , _unassembled_bytes_num(0)
    , _eof_idx(-1)
    , _output(capacity)
    , _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
    auto pos_iter = _unassemble_strs.upper_bound(index);
    if (pos_iter != _unassemble_strs.begin()) {
        pos_iter--;
    }

    size_t new_idx = index;
    if (pos_iter != _unassemble_strs.end() && pos_iter->first <= index) {
        const size_t up_idx = pos_iter->first;
        if (index < up_idx + pos_iter->second.size()) {
            new_idx = up_idx + pos_iter->second.size();
        }
    } else if (index < _next_assembled_idx) {
        new_idx = _next_assembled_idx;
    }

    const size_t data_start_pos = new_idx - index;
    ssize_t data_size = data.size() - data_start_pos;

    pos_iter = _unassemble_strs.lower_bound(new_idx);

    while (pos_iter != _unassemble_strs.end() && new_idx <= pos_iter->first) {
        const size_t data_end_pos = new_idx + data_size;
        if (pos_iter->first < data_end_pos) {
            if (data_end_pos < pos_iter->first + pos_iter->second.size()) {
                data_size = pos_iter->first - new_idx;
                break;
            } else {
                _unassembled_bytes_num -= pos_iter->second.size();
                pos_iter = _unassemble_strs.erase(pos_iter);
                continue;
            }
        } else {
            break;
        }
    }
    size_t first_unacceptable_idx = _next_assembled_idx + _capacity - _output.buffer_size();
    if (first_unacceptable_idx <= new_idx) {
        return;
    }

    if (data_size > 0) {
        const std::string new_data = data.substr(data_start_pos, data_size);
        if (new_idx == _next_assembled_idx) {
            const size_t write_byte = _output.write(new_data);
            _next_assembled_idx += write_byte;
            if (write_byte < new_data.size()) {
                const std::string data_to_store = new_data.substr(write_byte, new_data.size() - write_byte);
                _unassembled_bytes_num += data_to_store.size();
                _unassemble_strs.insert(make_pair(_next_assembled_idx, std::move(data_to_store)));
            }
        } else {
            const std::string data_to_store = new_data.substr(0, new_data.size());
            _unassembled_bytes_num += data_to_store.size();
            _unassemble_strs.insert(make_pair(new_idx, std::move(data_to_store)));
        }
    }

    for (auto iter = _unassemble_strs.begin(); iter != _unassemble_strs.end(); /* nop */) {
        assert(_next_assembled_idx <= iter->first);
        if (iter->first == _next_assembled_idx) {
            const size_t write_num = _output.write(iter->second);
            _next_assembled_idx += write_num;
            if (write_num < iter->second.size()) {
                _unassembled_bytes_num += iter->second.size() - write_num;
                _unassemble_strs.insert(make_pair(_next_assembled_idx, iter->second.substr(write_num)));

                _unassembled_bytes_num -= iter->second.size();
                _unassemble_strs.erase(iter);
                break;
            }
            _unassembled_bytes_num -= iter->second.size();
            iter = _unassemble_strs.erase(iter);
        } else {
            break;
        }
    }
    if (eof) {
        _eof_idx = index + data.size();
    }
    if (_eof_idx <= _next_assembled_idx) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_num; }

bool StreamReassembler::empty() const { return _unassembled_bytes_num == 0; }
