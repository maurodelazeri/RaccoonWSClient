cmake_minimum_required(VERSION 3.14)
project(RaccoonWSClient)

set(CMAKE_CXX_STANDARD 17)


if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else()
    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif()

add_executable(RaccoonWSClient main.cpp WsRaccoonClient.cc WsRaccoonClient.h queue.h)

target_link_libraries(RaccoonWSClient ${CONAN_LIBS})
