# Lab 3 Writeup

> [Checkpoint 3](https://cs144.github.io/assignments/lab3.pdf):
> the TCP sender

## TCP Sender

TCPSender: byte stream -> segments

The basic principle is to send whatever the receiver will allow us to send
(filling the window), and keep retransmitting until the receiver acknowledges
each segment.

1. Send SYN & Receive ack for initialization
2. Send data to fill window
3. Resend data if timer is out

* fill_window: reads from its input ByteStream and sends as many bytes as
  possible in the form of TCPSegments
* ack_received: updates outstanding segments
* tick: might retransmit a segment
* send_empty_segment: empty ack segment
