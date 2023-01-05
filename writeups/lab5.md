# Lab 5 Writeup

> [Checkpoint 5](https://cs144.github.io/assignments/lab5.pdf):
> the network interface

## Prepare

What has Linux provided?

* UDP socket
* TUN Device: network-level device, writing Ethernet header and sending to physical
  Ethernet card (exchange IP datagrams)
* TAP Device: link-level device, exchange link-layer frames

How are these segments actually conveyed to the peer’s TCP implementation?

* *TCP*-in-*UDP*-in-*IP*: *TCP* segments as payload in Linux-provided *UDP*
* *TCP*-in-*IP*: *TCP* segment/header with direct *IP* header, sending by TUN
  device
* *TCP*-in-*IP*-in-*Ethernet*: more than IP datagrams, link-layer frames (MAC
  address), sending by TAP device

---

In this lab, we need to implement the network interface, that is in between IP
protocol and link-layer device. -- Most of the work will be in looking up (and
caching) the Ethernet address for each next-hop IP address.
The protocol for this is called the **Address Resolution Protocol**, or ARP.

## network interface

* Send *datagram*
  * If the destination Ethernet address is *already known*, **send it right
    away**.<br/>
    Create an Ethernet frame (IPV4), set the payload to be the serialized
    datagram, and set the source and destination addresses.
  * If the destination Ethernet address is *unknown*, **broadcast** an ARP
    request for the next hop's Ethernet address, and **queue** the IP datagram,
    so it can be sent after the ARP reply is received.
  * **Do not send a 2nd request in 5 seconds**, just wait for a reply.
* Receive *frame*
  * If the inbound frame is *IPv4*, parse the payload as an InternetDatagram,
    and return the resulting InternetDatagram to the caller.
  * If the inbound frame is *ARP*, parse the payload as an ARPMessage, and
    **remember** the mapping between the sender’s IP address and Ethernet
    address for **30 seconds**. (Learn mappings from both requests and replies.)
    In addition, if it’s an ARP request asking for our IP address, send an
    appropriate ARP reply.
* When time passes: Expire any IP-to-Ethernet mappings that have expired
