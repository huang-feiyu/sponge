#include "router.hh"

#include <iostream>

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const std::optional<Address> next_hop,
                       const size_t interface_num) {
    std::cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
              << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num
              << "\n";
    _route_table.emplace_back(route_item{route_prefix, prefix_length, next_hop, interface_num});
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    auto matched = false;
    auto dst_ip = dgram.header().dst;
    route_item item;
    for (auto itr : _route_table) {
        auto mask = itr.prefix_length == 0 ? 0 : 0xffffffff << (32 - itr.prefix_length);
        if ((itr.route_prefix & mask) == (dst_ip & mask)) {
            if (!matched || item.prefix_length < itr.prefix_length) {
                matched = true;
                item = itr;
            }
        }
    }
    if (!matched || dgram.header().ttl <= 1) {
        return;
    }
    dgram.header().ttl--;
    if (item.next_hop.has_value()) {
        interface(item.interface_num).send_datagram(dgram, item.next_hop.value());
    } else {
        interface(item.interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dst_ip));
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
