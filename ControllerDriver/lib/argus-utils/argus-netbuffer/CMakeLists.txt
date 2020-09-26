cmake_minimum_required(VERSION 3.13)  # CMake version check
project(argus-netbuffer)              # Create project "argus-netbuffer"
set(CMAKE_CXX_STANDARD 17)            # Enable c++17 standard


add_library(argus-libnetbuffer STATIC)
target_sources(argus-libnetbuffer PRIVATE netbuffer.h netbuffer.cpp BasicMessageProtocol/BasicMessageProtocol.h BasicMessageProtocol/BasicMessageProtocol.cpp netutils.h netutils.cpp)
target_include_directories(argus-libnetbuffer PUBLIC ../)
