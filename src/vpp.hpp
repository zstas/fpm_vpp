#ifndef VPP_API_HPP_
#define VPP_API_HPP_

#include "vapi/vapi.hpp"
#include "vapi/vpe.api.vapi.hpp"
#include "vapi/ip.api.vapi.hpp"
#include "vapi/tapv2.api.vapi.hpp"

struct RouteMsg;

struct VPPAPI {
    vapi::Connection con;
    VPPAPI();
    ~VPPAPI();

    bool add_route( const RouteMsg &msg, bool add );
    bool create_tap( uint8_t id, const std::string &netns );
};

#endif