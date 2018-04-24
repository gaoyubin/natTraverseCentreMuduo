cmake_minimum_required(VERSION 3.8)
project(natTraverseCentreMuduo)

set(CMAKE_CXX_STANDARD 11)


#set(CMAKE_EXE_LINKER_FLAGS "-static")

set(SOURCE_FILES main.cpp CentreServer.cc  TcpTurnServer.cc UdpTurnServer.cpp UdpTurnServer.hpp)
include_directories(./release-install/include)
link_directories("./release-install/lib")


link_libraries(muduo_base muduo_net  pthread json)
add_executable(natTraverseCentreMuduo ${SOURCE_FILES})
