cmake_minimum_required(VERSION 3.15)
project(network C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable sanitizers for debug build" OFF)

# Warning flags
set(WARNING_FLAGS
    -Wall -Wextra -Wcast-align -Wcast-qual -Wchar-subscripts -Wconversion
    -Wempty-body -Wfloat-equal -Wformat-security -Wformat=2 -Wlogical-op
    -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow
    -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused
    -Wno-missing-field-initializers -Wno-narrowing
)

# Build type flags
set(CMAKE_C_FLAGS_DEBUG   "-D_DEBUG -ggdb3 -O0 -fstack-protector -fno-omit-frame-pointer -Werror=vla")
set(CMAKE_C_FLAGS_RELEASE "-O2")

# Sanitizers
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SAN_FLAGS
        -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow
        -fsanitize=float-divide-by-zero,integer-divide-by-zero,leak
        -fsanitize=nonnull-attribute,null,object-size,return,shift
        -fsanitize=signed-integer-overflow,undefined,unreachable,vla-bound
    )
endif()

# Find threads library
find_package(Threads REQUIRED)

# Logger library
add_library(logger STATIC
    logger/log.c
)
target_include_directories(logger PUBLIC logger)

# Utils library
add_library(utils STATIC
    utils/src/serializer.c
    utils/src/udp_discovery.c
)
target_include_directories(utils PUBLIC utils/include)
target_link_libraries(utils PUBLIC logger)

# Client library
add_library(client_lib STATIC
    client/src/tcp_client.c
    client/src/client.c
    client/src/config.c
)
target_include_directories(client_lib PUBLIC client/include utils/include logger)
target_link_libraries(client_lib PUBLIC utils logger Threads::Threads)

# Server library
add_library(server_lib STATIC
    server/src/tcp_server.c
)
target_include_directories(server_lib PUBLIC server/include utils/include logger)
target_link_libraries(server_lib PUBLIC utils logger Threads::Threads)

# Client executable
add_executable(client
    client/src/client_main.c
)
target_link_libraries(client PRIVATE client_lib Threads::Threads)
target_compile_options(client PRIVATE ${WARNING_FLAGS})

# Server executable
add_executable(server
    server/src/server_main.c
)
target_link_libraries(server PRIVATE server_lib Threads::Threads m)
target_compile_options(server PRIVATE ${WARNING_FLAGS})

# Apply sanitizers if enabled
if(SAN_FLAGS)
    target_compile_options(client PRIVATE ${SAN_FLAGS})
    target_link_options(client PRIVATE ${SAN_FLAGS})
    target_compile_options(server PRIVATE ${SAN_FLAGS})
    target_link_options(server PRIVATE ${SAN_FLAGS})
endif()
