#include "main.hpp"
#include <iomanip>

extern "C" {
#include "fpm.h"
}

int Netlink::process_route_msg( const struct nlmsghdr *nlh, void *data ) {
    struct rtmsg *rm = reinterpret_cast<struct rtmsg *>( NLMSG_DATA( nlh ) );
    switch( rm->rtm_family ) {
    case AF_INET: std::cout << "Family: AF_INET" << std::endl;
    case AF_INET6: std::cout << "Family: AF_INET6" << std::endl;
    }

    // struct nlattr *attr = reinterpret_cast<struct nlattr *>( (void*)rm + sizeof(rm) );
    auto attr = RTM_RTA( rm );
    int len = MNL_ALIGN( nlh->nlmsg_len );
    while( RTA_OK( attr, len ) ) {
        std::cout << "rtattr type: " << attr->rta_type << std::endl;
        switch( attr->rta_type ) {
        case RTA_DST:
            printf( "Destination: 0x%08x\n", *(uint32_t*)RTA_DATA( attr ) );
            break;
        case RTA_GATEWAY:
            printf( "Gateway: 0x%08x\n", *(uint32_t*)RTA_DATA( attr )  );
            break;
        case RTA_PRIORITY:
            printf( "Priority: %d\n", *(uint32_t*)RTA_DATA( attr )  );
            break;
        case RTA_OIF:
            printf( "OIF: %d\n", *(uint32_t*)RTA_DATA( attr )  );
            break;
        }
        attr = RTA_NEXT( attr, len );
    }
    return MNL_CB_OK;
}

int Netlink::data_cb( const struct nlmsghdr *nlh, void *data )
{
    
	switch( nlh->nlmsg_type ) {
	case RTM_NEWROUTE:
	case RTM_DELROUTE:
        std::cout << "ROUTE" << std::endl;
        return process_route_msg( nlh, data );
	case RTM_NEWNEIGH:
	case RTM_DELNEIGH:
        std::cout << "NEIGH" << std::endl;
        break;
		//return data_cb_neighbor(nlh, data);
	case RTM_NEWADDR:
	case RTM_DELADDR:
        std::cout << "ADDR" << std::endl;
        break;
		//return data_cb_address(nlh, data);
	default:
        std::cout << "Unknown type: " << nlh->nlmsg_type << std::endl;
        break;
	}
    // std::cout << "len: " << NLMSG_LENGTH( nlh ) << std::endl
    return MNL_CB_OK;
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
    if( hdr->msg_type == FPM_MSG_TYPE_NETLINK ) {
        std::cout << "Processing netlink fpm message" << std::endl;
        auto msg_data = fpm_msg_data( hdr );
        auto msg_len = fpm_msg_len( hdr );
        auto nh = (struct nlmsghdr *)msg_data;
        while( NLMSG_OK( nh, msg_len ) ) {
            std::cout << "NLMSG header type: " << nh->nlmsg_type << std::endl;
            /* The end of multipart message */
            if (nh->nlmsg_type == NLMSG_DONE)
                return;

            if (nh->nlmsg_type == NLMSG_ERROR)
                return;
            // void *data = NLMSG_PAYLOAD( nh, msg_len );
            data_cb( nh, NLMSG_DATA( nh ) );

            nh = NLMSG_NEXT( nh, msg_len );
        }

    } else if( hdr->msg_type == FPM_MSG_TYPE_PROTOBUF ) {
        std::cout << "We don't support protobug" << std::endl;
        // m.ParseFromArray( );
    } else {
        std::cout << "Unknown FPM MSG TYPE: " << static_cast<int>( hdr->msg_type ) << std::endl;
    }
}
