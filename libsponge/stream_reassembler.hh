#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <set>
#include <string>
#include <unordered_set>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    struct substring {
        size_t begin = 0;
        size_t length = 0;
        std::string data = "";
        bool operator<(const substring t) const { return begin < t.begin; }
    };
    std::set<substring> _substrings = {};  //!< The set of unassembled substrings
    size_t _unassembled_bytes = 0;         //!< The number of bytes in the substrings stored but not yet reassembled
    size_t _next_index = 0;                //!< The next index to write to
    bool _eof_flag = false;                //!< Flag indicates whether received eof

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    size_t merge(substring &elm1, const substring &elm2) {
        substring x, y;
        if (elm1.begin > elm2.begin) {
            x = elm2;
            y = elm1;
        } else {
            x = elm1;
            y = elm2;
        }
        if (x.begin + x.length < y.begin) {
            return -1;
        } else if (x.begin + x.length >= y.begin + y.length) {
            elm1 = x;
            return y.length;
        } else {
            elm1.begin = x.begin;
            elm1.data = x.data + y.data.substr(x.begin + x.length - y.begin);
            elm1.length = elm1.data.length();
            return x.begin + x.length - y.begin;
        }
    }

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
