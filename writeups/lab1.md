# Lab 1 Writeup

> [Checkpoint 1](https://cs144.github.io/assignments/lab1.pdf):
> stitching substrings into a byte stream

## Prepare

We will implement a TCP to provide the byte-stream abstraction between a pair of
computers separated by an unreliable datagram network.

* Lab 0: ByteStream
* Lab 1: StreamReassembler, stitches substrings into contiguous inorder byte
  stream
* Lab 2: TCPReceiver, handles inbound byte stream
* Lab 3: TCPSender, handles outbound byte stream
* Lab 4: TCPConnection, combine to create a working TCP

![Sponge TCP](https://lzx-figure-bed.obs.dualstack.cn-north-4.myhuaweicloud.com/Figurebed/202201222256243.png)

## Implementation

What we need to do is to implement StreamReassembler, which receive substrings,
consisting of a string of bytes, and the index of the first byte of that string
within the larger stream.

* Solution 1: Use a huge contiguous string buffer
