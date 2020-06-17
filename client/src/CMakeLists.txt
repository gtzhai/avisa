include_directories(.)
include_directories(../include)

# Build the asio library
set (ASIO_DIR ../../3rd/asio)
include_directories (${ASIO_DIR}/include)

# the websocket library
set (WEBSOCKET_DIR ../../3rd/websocketpp)
add_definitions(-DASIO_STANDALONE)
include_directories (${WEBSOCKET_DIR}/)

set(lib_avisa_SOURCES visa_base_wrapper.cc ResManager.cpp SessionManager.cpp IdManager.cpp)
add_library(avisa ${lib_avisa_SOURCES})
target_link_libraries(avisa pthread z)

set(client_SOURCES main.c)
add_executable(client ${client_SOURCES})
target_link_libraries(client m avisa)

