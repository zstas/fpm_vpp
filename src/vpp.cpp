#include "main.hpp"

DEFINE_VAPI_MSG_IDS_VPE_API_JSON
DEFINE_VAPI_MSG_IDS_IP_API_JSON
DEFINE_VAPI_MSG_IDS_TAPV2_API_JSON

VPPAPI::VPPAPI() {
    log( "VPPAPI cstr" );
    auto ret = con.connect( "fibmgr", nullptr, 32, 32 );
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
    req.route.table_id = msg.vrf_id;
    req.is_multipath = 0;

    if( msg.af == AF_INET ) {
        req.route.prefix.address.af = vapi_enum_address_family::ADDRESS_IP4;
        req.route.paths->proto = vapi_enum_fib_path_nh_proto::FIB_API_PATH_NH_PROTO_IP4;
        std::copy( msg.destination.begin(), msg.destination.end(), req.route.prefix.address.un.ip4 );
        req.route.prefix.len = msg.dest_len;

        // Filling up the nexthop info
        req.route.n_paths = msg.nhops.size();
        uint8_t i = 0;
        for( auto const &nh: msg.nhops ) {
            if( !nh.nexthop.empty() ) {
                req.route.paths[i].sw_if_index = ~0;
                std::copy( nh.nexthop.begin(), nh.nexthop.end(), req.route.paths[i].nh.address.ip4 );
                i++;
            }
        }
    } else if( msg.af == AF_INET6 ) {
        req.route.prefix.address.af = vapi_enum_address_family::ADDRESS_IP6;
        req.route.paths->proto = vapi_enum_fib_path_nh_proto::FIB_API_PATH_NH_PROTO_IP6;
        std::copy( msg.destination.begin(), msg.destination.end(), req.route.prefix.address.un.ip6 );
        req.route.prefix.len = msg.dest_len;

        // Filling up the nexthop info
        req.route.n_paths = msg.nhops.size();
        uint8_t i = 0;
        for( auto const &nh: msg.nhops ) {
            if( !nh.nexthop.empty() ) {
                req.route.paths[i].sw_if_index = ~0;
                std::copy( nh.nexthop.begin(), nh.nexthop.end(), req.route.paths[i].nh.address.ip6 );
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

bool VPPAPI::set_vrf( const std::string &name, uint32_t id, bool is_add, bool is_v6 ) {
    vapi::Ip_table_add_del table { con };

    auto &req = table.get_request().get_payload();
    req.table.is_ip6 = is_v6 ? 1 : 0;
    req.table.table_id = id;
    if( is_add ) {
        std::memset( req.table.name, 0, sizeof( req.table.name ) );
        std::copy( name.begin(), name.end(), req.table.name );
        req.is_add = 1;
    } else {
        req.is_add = 0;
    }

    auto ret = table.execute();
    if( ret != VAPI_OK ) {
        return false;
    }

    do {
        ret = con.wait_for_response( table );
    } while( ret == VAPI_EAGAIN );

    auto &repl = table.get_response().get_payload();
    if( repl.retval < 0 ) {
        return false;
    }

    vrfs.emplace( name, id );

    return true;
}

