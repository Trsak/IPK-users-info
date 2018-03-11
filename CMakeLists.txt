cmake_minimum_required(VERSION 3.6)
project(ipk)
set(CMAKE_CXX_STANDARD 11)

set(CLIENT_FILES client.cpp)
add_executable(ipk-client ${CLIENT_FILES})

set(SERVER_FILES server.cpp)
add_executable(upk-server ${SERVER_FILES})  