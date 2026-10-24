add_library(ocnet STATIC
    net_protocol.cpp
    net_loopback.cpp
    embedded_server.cpp
)
target_link_libraries(ocnet PUBLIC ocbase)
