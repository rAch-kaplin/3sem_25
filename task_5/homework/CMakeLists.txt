cmake_minimum_required(VERSION 3.15)
project(signal_project C)

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

set(CMAKE_C_FLAGS_DEBUG   "-D_DEBUG -ggdb3 -O0 -fstack-protector -fno-omit-frame-pointer -Werror=vla")
set(CMAKE_C_FLAGS_RELEASE "-O2")

if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SAN_FLAGS
        -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow
        -fsanitize=float-divide-by-zero,integer-divide-by-zero,leak
        -fsanitize=nonnull-attribute,null,object-size,return,shift
        -fsanitize=signed-integer-overflow,undefined,unreachable,vla-bound
    )
endif()

file(GLOB SOURCES
    "src/*.c"
)

file(GLOB HEADERS
    "include/*.h"
)

add_executable(run ${SOURCES})

target_include_directories(run PRIVATE include logger)
target_compile_options(run PRIVATE ${WARNING_FLAGS})
target_link_libraries(run PRIVATE magic)

if(SAN_FLAGS)
    target_compile_options(run PRIVATE ${SAN_FLAGS})
    target_link_options(run PRIVATE ${SAN_FLAGS})
endif()
