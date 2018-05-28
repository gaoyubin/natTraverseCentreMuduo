cmake_minimum_required(VERSION 3.8)
project(natTraverseCentreMuduo)

add_definitions("-g")
set(CMAKE_CXX_STANDARD 11)


#set(CMAKE_EXE_LINKER_FLAGS "-static")

set(SOURCE_FILES main.cpp CentreServer.cc  TcpTurnServer.cc UdpTurnServer.cpp UdpTurnServer.hpp)
include_directories(./release-install/include)
link_directories("./release-install/lib")


link_libraries(muduo_base muduo_net  pthread json)
execute_process(
                #COMMAND touch aaa.txt
                #COMMAND ls -l
                #OUTPUT_FILE aaa.txt
        COMMAND sshpass -p tl scp natTraverseCentreMuduo tl@218.192.170.178:/home/tl/project/test
        )

add_executable(natTraverseCentreMuduo ${SOURCE_FILES})