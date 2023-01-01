# Lab 2 Writeup

> [Checkpoint 2](https://cs144.github.io/assignments/lab2.pdf):
> the TCP receiver

## Prepare

TCP receiver is not only responsible for writing to the incoming stream, but
also for telling sender window (a range of indexes, constructed by ackno and
window size). Using window, the receiver can control the flow of incoming data.

* receiving TCP segments
* reassembling the byte stream
* determining that signals that should be sent back to the sender for
  acknowledgment and flow control

## Task #1: Translating between 64-bit indexes and 32-bit seqnos

| Sequence Numbers  | Absolute Sequence Numbers | Stream Indices        |
|-------------------|---------------------------|-----------------------|
| Start at the ISN  | Start at 0                | Start at 0            |
| Include SYN/FIN   | Include SYN/FIN           | Omit SYN/FIN          |
| 32 bits, wrapping | 64 bits, non-wrapping     | 64 bits, non-wrapping |
| "seqno"           | "absolute seqno"          | "stream index"        |

* `WrappingInt32 wrap(uint64_t n, WrappingInt32 isn)`: Convert *Absolute Seqno*
  → *Seqno*
* `uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint)`:
  Convert *Seqno* → *Absolute Seqno*
