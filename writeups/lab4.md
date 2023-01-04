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

---

3-Way handshake [normal]:
```
***Xxx***: peer role
XXXXX    : state
XXX -> XX: state transiting
{XXX}    : transmitting segment

***Client***                   ***Server***
CLOSED                         LISTEN
       --------{SYN}-------->                  handshake#1
SYN SENT                       LISTEN -> SYN RECV

       <-----{SYN/ACK}-------                  handshake#2
SYN SENT -> ESTABLISHED        SYN RECV

       --------{ACK}-------->                  handshake#3
ESTABLISHED                    SYN RECV -> ESTABLISHED
```

3-Way handshake [simultaneous open]:
```
***Peer1***                   ***Peer2***
CLOSED                         LISTEN
       --------{SYN}-------->                  handshake#1 [1->2]
       <-------{SYN}---------                  handshake#1 [2->1]
SYN SENT                       SYN SENT

# both ***Peer1*** and ***Peer2*** receive {SYN} in SYN SENT
# => ***Peer1*** & ***Peer2***: SYN SENT -> SYN RECV, and to send handshake#3-like to each other

SYN RECV                       SYN RECV
       --------{ACK}-------->                  handshake#3-like [1->2]
       <-------{ACK}---------                  handshake#3-like [2->1]
SYN RECV -> ESTABLISHED        SYN RECV -> ESTABLISHED
```

## Implementation

1. Wire up ordinary methods to *receiver* & *sender*
2. Send segments: send SYN first, send data, send FIN with "best effort"
3. Receive segments: state machine
4. When time passes, update *sender* and shutdown if necessary

---

![State machine](https://user-images.githubusercontent.com/70138429/210497471-3a873eb8-f394-4642-ad8f-e7b0dccbc08b.png)

![More precise](http://tcpipguide.com/free/diagrams/tcpfsm.png)
