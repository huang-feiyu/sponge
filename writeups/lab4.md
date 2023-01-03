# Lab 4 Writeup

> [Checkpoint 4](https://cs144.github.io/assignments/lab4.pdf):
> the TCP connection

## TCP connection

**!Important**: to test send packets really, we need to turn off firewall by
`sudo systemctl stop firewalld`.

A *TCP connection* is considered as a peer. It's responsible for receiving and
sending segments, making sure the sender and receiver are informed about and
have a chance to contribute to the fields they care about for incoming and
outgoing segments.
Two parties (receiver & sender) participate in the TCP connection, and each
party acts as both "sender" (of its own outbound byte-stream) and "receiver" (of
an inbound byte-stream) at the same time.

* Receiving segments
  * if `RST`: connection is done
  * check normal fields `seqno`, `SYN`, `payload`, `FIN` in *receiver*
  * if `ACK`: update *sender* states with `ackno` and `window_size`
* Sending segments
  * set normal fields `seqno`, `SYN`, `payload`, `FIN` in *sender*
  * set `ACK`, `ackno`, `window_size` if possible (has established connection)
    in *receiver*
* When time passes
  * update *sender*
  * send `RST` if reaches max_attempts or lingers for a long time
