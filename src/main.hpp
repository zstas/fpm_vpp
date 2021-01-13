#include <boost/asio.hpp>
#include <iostream>
#include <queue>

#include <linux/rtnetlink.h>

#include "vpp.hpp"
#include "netlink.hpp"

extern void log( const std::string &m );