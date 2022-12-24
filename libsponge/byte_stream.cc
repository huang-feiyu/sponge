#include "byte_stream.hh"

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const std::string &data) {
    auto length = data.size() > remaining_capacity() ? remaining_capacity() : data.size();
    _w_cnt += length;
    for (size_t i = 0; i < length; i++) {
        _buffer.push_back(data[i]);
    }
    return length;
}

//! \param[in] len bytes will be copied from the output side of the buffer
std::string ByteStream::peek_output(const size_t len) const {
    auto length = len > buffer_size() ? buffer_size() : len;
    return std::string().assign(_buffer.begin(), _buffer.begin() + length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    auto length = len > buffer_size() ? buffer_size() : len;
    for (size_t i = 0; i < length; i++) {
        _buffer.pop_front();
    }
    _r_cnt += length;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // peek then pop
    auto res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _w_cnt; }

size_t ByteStream::bytes_read() const { return _r_cnt; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
