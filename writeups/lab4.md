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

In general, *TCP connection* will do:
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

## Implementation

1. Wire up ordinary methods to *receiver* & *sender*
2. Send segments: send SYN first, send data, send FIN with "best effort"
3. Receive segments: state machine
4. When time passes, update *sender* and shutdown if necessary

---

![State machine](https://user-images.githubusercontent.com/70138429/210497471-3a873eb8-f394-4642-ad8f-e7b0dccbc08b.png)

![More precise](http://tcpipguide.com/free/diagrams/tcpfsm.png)

### 3-Way handshake

3-Way handshake [normal]:
```
***Xxx***: peer role
XXXXX    : state
XXX -> XX: state transiting
{XXX}    : transmitting segment

***Client***                       ***Server***
CLOSED                             LISTEN
            --------{SYN}-------->                  handshake#1
SYN SENT                           LISTEN
                                   -> SYN RECV

            <-----{SYN/ACK}-------                  handshake#2
SYN SENT                           SYN RECV
-> ESTAB

            --------{ACK}-------->                  handshake#3
ESTABLISHED                        SYN RECV
                                   -> ESTAB
```

3-Way handshake [simultaneous open]:
```
# both ***Peer1*** and ***Peer2*** receive {SYN} in SYN SENT
# => ***Peer1*** & ***Peer2***: SYN SENT -> SYN RECV, and to send handshake#3-like to each other

***Peer1***                       ***Peer2***
CLOSED                            LISTEN
           --------{SYN}-------->                  handshake#1 [1->2]
           <-------{SYN}---------                  handshake#1 [2->1]
SYN SENT                          SYN SENT
-> SYN RECV                       -> SYN RECV

           --------{ACK}-------->                  handshake#3-like [1->2]
           <-------{ACK}---------                  handshake#3-like [2->1]
SYN RECV                          SYN RECV
-> ESTAB                          -> ESTAB
```

### 4-Way handshake

To distinguish with previous, I call it **4-Way goodbye**.
More: why there is one more handshake?<br/>
=> Previous one merge ACK/SYN together, but we need to wait for CLOSE/FIN in 
when goodbye.

4-Way goodbye [normal]:
```
***Client***                       ***Server***
ESTAB                              ESTAB
           --------{FIN}-------->                  goodbye#1
FIN_WAIT_1                         ESTAB
                                   -> CLOSE WAIT

           <-------{ACK}---------                  goodbye#2
FIN WAIT 1                         CLOSE WAIT
-> FIN WAIT 2

#=== ***Client*** still receive data from ***Server*** ===#

          <--------{FIN}---------                  goodbye#3
FIN WAIT 2                         CLOSE WAIT
-> TIME WAIT                       -> LAST ACK

          ---------{ACK}-------->                  goodbye#4
TIME WAIT                          LAST ACK
    |                              -> CLOSED
    V
  timeout
CLOSED
```

4-Way goodbye [simultaneous close]:
```
# both ***Peer1*** and ***Peer2*** receive {FIN} in FIN WAIT 1
# => ***Peer1*** & ***Peer2***: FIN WAIT 1 -> CLOSING, and to send goodbye#4-like to each other

***Peer1***                       ***Peer2***
ESTAB                              ESTAB
           --------{FIN}-------->                  goodbye#1 [1->2]
           <-------{FIN}---------                  goodbye#1 [2->1]
FIN WAIT 1                         FIN WAIT 1
-> CLOSING                         -> CLOSING

           --------{ACK}-------->                  goodbye#4-like [1->2]
           <-------{ACK}---------                  goodbye#4-like [2->1]
CLOSING                            CLOSING
-> TIME WAIT                       -> TIME WAIT
    |                                  |
    V                                  V
  timeout                            timeout
CLOSED                             CLOSED
```

## Performance Optimization

Too slow to show *tcp_benchmark* => Update stream reassembler.

```
CPU-limited throughput                : 1.10 Gbit/s
CPU-limited throughput with reordering: 1.04 Gbit/s
```

Update [./etc/cflags.cmake](../etc/cflags.cmake) "-g" to "-Og -pg", and run
`gprof ./apps/tcp_benchmark > prof.txt` to find out what effects most.

```
 32.82      0.43     0.43   219544     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<std::_Deque_iterator<char, char const&, char const*> >(std::_Deque_iterator<char, char const&, char const*>, std::_Deque_iterator<char, char const&, char const*>, std::forward_iterator_tag)
 31.30      0.84     0.41   219540     0.00     0.00  ByteStream::write(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
 26.72      1.19     0.35   219544     0.00     0.00  ByteStream::pop_output(unsigned long)
```

=> <s>Update byte stream.</s>
