#ifndef VPP_API_HPP_
#define VPP_API_HPP_

#include "vapi/vapi.hpp"
#include "vapi/vpe.api.vapi.hpp"
#include "vapi/ip.api.vapi.hpp"
#include "vapi/tapv2.api.vapi.hpp"

struct RouteMsg;

class VPPAPI {
public:
    VPPAPI();
    ~VPPAPI();

    bool add_route( const RouteMsg &msg, bool add );
    bool create_tap( uint8_t id, const std::string &netns );
    bool set_vrf( const std::string &name, uint32_t id, bool is_add, bool is_v6 );
private:
    vapi::Connection con;
    std::map<std::string,uint32_t> vrfs;
};

#endif