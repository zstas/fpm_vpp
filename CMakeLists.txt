cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(CMAKE_CXX_FLAGS_DEBUG "-O0")
SET(CMAKE_CXX_FLAGS_DEBUG "-g")

# set the project name
project(fibmgr)

file(GLOB SOURCES src/*.cpp)

# add the executable
add_executable(fibmgr ${SOURCES})

target_link_libraries(fibmgr PUBLIC boost_system)
target_link_libraries(fibmgr PUBLIC pthread)
target_link_libraries(fibmgr PUBLIC mnl)
target_link_libraries(fibmgr PUBLIC vapiclient)
target_link_libraries(fibmgr PUBLIC vppapiclient)