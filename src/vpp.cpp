#include "main.hpp"

DEFINE_VAPI_MSG_IDS_VPE_API_JSON
DEFINE_VAPI_MSG_IDS_IP_API_JSON
DEFINE_VAPI_MSG_IDS_TAPV2_API_JSON

VPPAPI::VPPAPI() {
    log( "VPPAPI cstr" );
    auto ret = con.connect( "vbng", nullptr, 32, 32 );
    if( ret == VAPI_OK ) {
        log("VPP API: connected");
    } else {
        log( "VPP API: Cannot connect to vpp" );
    }
}

VPPAPI::~VPPAPI() {
    auto ret = con.disconnect();
    if( ret == VAPI_OK ) {
        log("VPP API: disconnected");
    } else {
        log("VPP API: something went wrong, cannot disconnect");
    }
}

bool VPPAPI::add_route( const RouteMsg &msg, bool add ) {
    vapi::Ip_route_add_del route( con, 0 );
    auto &req = route.get_request().get_payload();
    req.is_add = add ? 1 : 0;

    if( !msg.is_ipv6 ) {
        req.route.prefix.address.af = vapi_enum_address_family::ADDRESS_IP4;
        req.route.table_id = msg.vrf_id;
        req.is_multipath = 0;

        // Filling up the route info
        struct in_addr ipv4;
        memset( &ipv4, 0, sizeof( ipv4 ) );
        inet_pton( AF_INET, msg.destination.c_str(), &ipv4 );
        *(uint32_t*)req.route.prefix.address.un.ip4 = ipv4.s_addr;
        req.route.prefix.len = msg.dest_len;

        // Filling up the nexthop info
        req.route.n_paths = msg.nhops.size();
        uint8_t i = 0;
        for( auto const &nh: msg.nhops ) {
            if( nh.nexthop ) {
                memset( &ipv4, 0, sizeof( ipv4 ) );
                inet_pton( AF_INET, msg.destination.c_str(), &ipv4 );
                *(uint32_t*)req.route.paths[i].nh.address.ip4 = ipv4.s_addr;
                i++;
            }
        }
    } else {
        return false;
    }

    auto ret = route.execute();
    if( ret != VAPI_OK ) {
        log( "error!" );
    }

    do {
        ret = con.wait_for_response( route );
    } while( ret == VAPI_EAGAIN );

    auto repl = route.get_response().get_payload();
    if( static_cast<int>( repl.stats_index ) == -1 ) {
        log( "cannot add route" );
        return false;
    }
    log( "successfully installed route: " + std::to_string( repl.stats_index ) );
    return true;
}

bool VPPAPI::create_tap( uint8_t id, const std::string &netns ) {
    vapi::Tap_create_v2 tap{ con };
    auto &req = tap.get_request().get_payload();
    //std::copy( netns.begin(), netns.end(), req.host_namespace );
    req.id = id;

    auto ret = tap.execute();
    if( ret != VAPI_OK ) {
        log( "error!" );
    }

    do {
        ret = con.wait_for_response( tap );
    } while( ret == VAPI_EAGAIN );

    auto repl = tap.get_response().get_payload();
    if( static_cast<int>( repl.sw_if_index ) == -1 ) {
        log( "cannot add tap" );
        return false;
    }
    log( "successfully added tap sw_if_index: " + std::to_string( repl.sw_if_index ) );
    return true;
}