cmake_minimum_required(VERSION 3.15)
project(daemon_project C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable sanitizers for debug build" OFF)

set(WARNING_FLAGS
    -Wall -Wextra -Wcast-align -Wcast-qual -Wchar-subscripts -Wconversion
    -Wempty-body -Wfloat-equal -Wformat-security -Wformat=2 -Wlogical-op
    -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow
    -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused
    -Wno-missing-field-initializers -Wno-narrowing
)

set(CMAKE_C_FLAGS_DEBUG "-D_DEBUG -ggdb3 -O0 -fstack-protector -fno-omit-frame-pointer -pie -fPIE -Werror=vla")
set(CMAKE_C_FLAGS_RELEASE "-O2")

if(ENABLE_SANITIZERS)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}
        -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow
        -fsanitize=float-divide-by-zero,integer-divide-by-zero,leak
        -fsanitize=nonnull-attribute,null,object-size,return,shift
        -fsanitize=signed-integer-overflow,undefined,unreachable,vla-bound
    ")
endif()

file(GLOB SOURCES
    "src/*.c"
    "logger/log.c"
)

file(GLOB HEADERS
    "include/*.h"
    "logger/log.h"
)

add_executable(daemon ${SOURCES} ${HEADERS})
target_include_directories(daemon PRIVATE include logger)
target_link_libraries(daemon PRIVATE magic)
target_compile_options(daemon PRIVATE ${WARNING_FLAGS})
