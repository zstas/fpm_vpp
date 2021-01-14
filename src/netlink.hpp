#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <iosfwd>
#include <optional>

struct RouteNhop {
    std::vector<uint8_t> nexthop;
    std::optional<uint32_t> oif;
    uint32_t priority{ 0U };
};

struct RouteMsg {
    int af;
    std::vector<uint8_t> destination;
    std::vector<RouteNhop> nhops;
    uint8_t dest_len{ 0U };
    uint8_t vrf_id{ 0U };
};

std::ostream& operator<<( std::ostream &os, const RouteMsg &m );

struct Netlink {
private:
    VPPAPI vpp;
public:
    Netlink() {
        // vpp.create_tap( 1, "bgp" );
    }
    RouteMsg process_route_msg( const struct nlmsghdr *nlh );
    bool process_one_msg( const struct nlmsghdr *nlh );
    void process( std::vector<uint8_t> &v);
};

#endif