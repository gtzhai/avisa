cmake_minimum_required(VERSION 3.4.1)

set (asio_sources
        src/asio.cpp
        )

add_library(asio ${asio_sources})

# Specify directories which the compiler should look for headers
include_directories(asio ./include)

# Compile Flags:
#     Enable -Werror when building debug config
#     Enable -Ofast
target_compile_options(asio
	PRIVATE
	-std=c++11
    -Wall
    -Wshadow
    -Ofast
	-DASIO_STANDALONE)

#target_link_libraries(asio)
