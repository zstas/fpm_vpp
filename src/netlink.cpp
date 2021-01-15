#include <iomanip>
#include <sstream>

#include "main.hpp"
#include "netlink.hpp"

extern "C" {
#include "fpm.h"
}

static std::string printIp( int af, const std::vector<uint8_t> &vec ) {
    struct in_addr ipv4;
    memset( &ipv4, 0, sizeof( ipv4 ) );
    struct in6_addr ipv6;
    memset( &ipv6, 0, sizeof( ipv6 ) );
    char buf[ INET6_ADDRSTRLEN ];
    memset( buf, 0, sizeof( buf ) );

    if( af == AF_INET ) {
        memcpy( (uint8_t*)&ipv4.s_addr, vec.data(), vec.size() );
        inet_ntop( af, &ipv4, buf, sizeof( buf ) );
    } else {
        memcpy( ipv6.__in6_u.__u6_addr8, vec.data(), vec.size() );
        inet_ntop( af, &ipv6, buf, sizeof( buf ) );
    }

    return { buf };
}

std::ostream& operator<<( std::ostream &os, const RouteMsg &m ) {
    os << "Destination: " << printIp( m.af, m.destination) << "/" << (int)m.dest_len << "(vec len: " << m.destination.size() << ")";
    os << " VRF ID: " << (int)m.vrf_id;
    for( auto const &nh: m.nhops ) {
        if( !nh.nexthop.empty() )
            os << " Nexthop: " << printIp( m.af, nh.nexthop ) << "(vec len: " << nh.nexthop.size() << ")";
        os << " Priority: " << nh.priority;
        os << " NHVrf: " << nh.vrf_id;
        if( nh.oif ) 
            os << " OIF: " << *nh.oif;

        if( nh.label ) 
            os << " MPLS Label: " << *nh.label;
    }
    return os;
}

RouteMsg Netlink::process_route_msg( const struct nlmsghdr *nlh ) {
    RouteMsg msg;
    RouteNhop nhop;
    struct rtmsg *rm = reinterpret_cast<struct rtmsg *>( NLMSG_DATA( nlh ) );
    msg.af = rm->rtm_family;
    msg.vrf_id = rm->rtm_table;

    auto attr = RTM_RTA( rm );
    int len = RTA_ALIGN( nlh->nlmsg_len );

    while( RTA_OK( attr, len ) ) {
        switch( attr->rta_type ) {
        case RTA_DST:
            msg.destination = std::vector<uint8_t>( (uint8_t*)RTA_DATA( attr ),  (uint8_t*)RTA_DATA( attr ) + RTA_PAYLOAD( attr ) );
            msg.dest_len = rm->rtm_dst_len;
            break;
        case RTA_GATEWAY:
            nhop.nexthop = std::vector<uint8_t>( (uint8_t*)RTA_DATA( attr ),  (uint8_t*)RTA_DATA( attr ) + RTA_PAYLOAD( attr ) );
            break;
        case RTA_PRIORITY:
            nhop.priority = *(uint32_t*)RTA_DATA( attr );
            break;
        case RTA_OIF:
            nhop.oif = *(uint32_t*)RTA_DATA( attr );
            break;
        case RTA_TABLE:
            nhop.vrf_id = *(uint32_t*)RTA_DATA( attr );
        case RTA_FLOW:
            nhop.label = *(uint32_t*)RTA_DATA( attr );
        case RTA_MULTIPATH: {
            struct rtnexthop *nhptr = (struct rtnexthop*)RTA_DATA( attr );
            int rtnhp_len = RTNH_ALIGN( attr->rta_len );

            while( RTNH_OK( nhptr, rtnhp_len ) ) {
                nhop = RouteNhop();
                struct rtattr *nhattr = RTNH_DATA( nhptr );
                int attrlen = RTA_ALIGN( nhattr->rta_len );
                while( RTA_OK( nhattr, attrlen ) ) {
                    switch( nhattr->rta_type ) {
                    case RTA_GATEWAY:
                        nhop.nexthop = std::vector<uint8_t>( (uint8_t*)RTA_DATA( attr ),  (uint8_t*)RTA_DATA( attr ) + RTA_PAYLOAD( attr ) );
                        break;
                    case RTA_PRIORITY:
                        nhop.priority = *(uint32_t*)RTA_DATA( nhattr );
                        break;
                    case RTA_OIF:
                        nhop.oif = *(uint32_t*)RTA_DATA( nhattr );
                        break;
                    case RTA_TABLE:
                        nhop.vrf_id = *(uint32_t*)RTA_DATA( nhattr );
                    case RTA_FLOW:
                        nhop.label = *(uint32_t*)RTA_DATA( nhattr );
                    }
                    nhattr = RTA_NEXT( nhattr, attrlen );
                }
                msg.nhops.push_back( nhop );
                nhptr = RTNH_NEXT( nhptr );
            }

            break;
        }
        default:
            printf( "Unknown attribute %d\n", attr->rta_type );
        }
        attr = RTA_NEXT( attr, len );
    }

    if( msg.nhops.empty() ) {
        msg.nhops.emplace_back( nhop );
    }

    return msg;
}

bool Netlink::process_one_msg( const struct nlmsghdr *nlh ) {
	switch( nlh->nlmsg_type ) {
	case RTM_NEWROUTE:
	case RTM_DELROUTE: {
        auto msg = process_route_msg( nlh );
        std::cout << "Processing route: " << msg << std::endl;
        vpp.add_route( msg, nlh->nlmsg_type == RTM_NEWROUTE ? true : false );
        break;
    }
	case RTM_NEWNEIGH:
	case RTM_DELNEIGH:
        std::cout << "NEIGH" << std::endl;
        break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
        std::cout << "ADDR" << std::endl;
        break;
	default:
        std::cout << "Unknown type: " << nlh->nlmsg_type << std::endl;
        break;
	}
    return true;
}

std::ostream& operator<<( std::ostream &os, const std::vector<uint8_t> &in ) {
    auto flags = os.flags();
    for( auto const &b: in ) {
        os << "0x" << std::hex << std::setw( 2 ) << std::setfill('0') << static_cast<int>( b ) << " ";
    }
    os.flags( flags );
    return os;
}

void Netlink::process( std::vector<uint8_t> &v) {
    int ret;
    fpm_msg_hdr_t *hdr;
    hdr = reinterpret_cast<fpm_msg_hdr_t *>( v.data() );
    auto len = v.size();
    while( fpm_msg_ok( hdr, len ) ) {
        if( hdr->msg_type == FPM_MSG_TYPE_NETLINK ) {
            // std::cout << "Processing netlink fpm message" << std::endl;
            auto msg_len = fpm_msg_len( hdr );

            struct nlmsghdr *nh = reinterpret_cast<struct nlmsghdr*>( fpm_msg_data( hdr ) );
            while( NLMSG_OK( nh, msg_len ) ) {
                /* The end of multipart message */
                if (nh->nlmsg_type == NLMSG_DONE)
                    return;

                if (nh->nlmsg_type == NLMSG_ERROR)
                    return;

                process_one_msg( nh );

                nh = NLMSG_NEXT( nh, msg_len );
            }

        } else if( hdr->msg_type == FPM_MSG_TYPE_PROTOBUF ) {
            std::cout << "We don't support protobug" << std::endl;
        } else {
            std::cout << "Unknown FPM MSG TYPE: " << static_cast<int>( hdr->msg_type ) << std::endl;
        }
        hdr = fpm_msg_next( hdr, &len );
    }
}
