include_directories(.)
# Build the asio library
set (ASIO_DIR ../3rd/asio)
include_directories (${ASIO_DIR}/include)

# the websocket library
set (WEBSOCKET_DIR ../3rd/websocketpp)
add_definitions(-DASIO_STANDALONE)
include_directories (${WEBSOCKET_DIR}/)

set(lib_avisas_SOURCES Finder.cpp ComFinder.cpp UsbFinder.cpp MdnsFinder.cpp UsbTmcPort.cpp
        VisaService.cpp 
        VisaWsServer.cpp 
        )
add_library(avisas ${lib_avisas_SOURCES})
target_link_libraries (avisas z)
target_link_libraries (avisas PRIVATE spdlog)
set(app_SOURCES main.cpp OsService.cpp)
add_executable(app ${app_SOURCES})
target_link_libraries(app avisas usb-1.0 pthread)

