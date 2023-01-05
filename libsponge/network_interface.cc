#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    std::cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
              << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // Find MAC according to IP addr
    auto itr = _arp_table.find(next_hop_ip);

    if (itr != _arp_table.end()) {
        // encapsulate datagram and send frame right now
        EthernetFrame frame;
        frame.header().type = frame.header().TYPE_IPv4;
        frame.header().src = _ethernet_address;
        frame.header().dst = itr->second.mac;
        frame.payload() = dgram.serialize();
        _frames_out.emplace(frame);
    } else {
        // broadcast ARP request and queue IP datagram until knowing where to send it
        if (_waiting_ips.find(next_hop_ip) == _waiting_ips.end()) {
            ARPMessage arp_req;
            arp_req.opcode = ARPMessage::OPCODE_REQUEST;
            arp_req.sender_ethernet_address = _ethernet_address;
            arp_req.sender_ip_address = _ip_address.ipv4_numeric();
            arp_req.target_ethernet_address = ETHERNET_BROADCAST;  // placeholder
            arp_req.target_ip_address = next_hop_ip;

            EthernetFrame frame;
            frame.header().type = frame.header().TYPE_ARP;
            frame.header().src = _ethernet_address;
            frame.header().dst = ETHERNET_BROADCAST;  // placeholder
            frame.payload() = arp_req.serialize();
            _frames_out.emplace(frame);

            _waiting_ips[next_hop_ip] = _default_arp_req_ttl;
        }
        _waiting_datagrams.emplace_back(std::make_pair(next_hop, dgram));
    }
}

//! \param[in] frame the incoming Ethernet frame
std::optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) { return {}; }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {}
