cmake_minimum_required(VERSION 3.10)
project(monte_carlo_distributed)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(COMMON_SOURCES
    src/common.c
)

set(MONTE_CARLO_SOURCES
    monte_carlo/monte_carlo.c
)

set(CLIENT_SOURCES
    src/client_main.c
    src/udp_discovery.c
    src/tcp_client.c
)

set(SERVER_SOURCES
    src/server_main.c
    src/udp_discovery.c
    src/tcp_server.c
)

add_executable(client
    ${CLIENT_SOURCES}
    ${COMMON_SOURCES}
    ${MONTE_CARLO_SOURCES}
)

target_include_directories(client PRIVATE
    include
    monte_carlo
)

target_link_libraries(client
    pthread
    m
)

add_executable(server
    ${SERVER_SOURCES}
    ${COMMON_SOURCES}
    ${MONTE_CARLO_SOURCES}
)

target_include_directories(server PRIVATE
    include
    monte_carlo
)

target_link_libraries(server
    pthread
    m
)
