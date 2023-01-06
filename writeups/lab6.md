# Lab 6 Writeup

> [Checkpoint 6](https://cs144.github.io/assignments/lab6.pdf):
> the IP router

## IP Router

A *router* has several *network interface*s, and can receive Internet datagrams
on any of them. The router's job is to forward the datagrams it gets according
to the routing table: a list of rules that tells the router, for any given
datagram.

What we need to do is to a router that can figure out:
* keep track of a routing table (the list of forwarding rules, or routes), and
* forward each datagram it receives:
  * to the correct next hop
  * on the correct outgoing NetworkInterface.
