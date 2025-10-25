cmake_minimum_required(VERSION 3.15)

option(ENABLE_SANITIZERS "Enable sanitizers for debug build" OFF)

add_compile_options(
    -Wall -Wextra -Wcast-align -Wcast-qual -Wchar-subscripts -Wconversion
    -Wempty-body -Wfloat-equal -Wformat-security -Wformat=2 -Wlogical-op
    -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow
    -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused
    -Wno-missing-field-initializers -Wno-narrowing
)

set(CMAKE_C_FLAGS_DEBUG "-D_DEBUG -ggdb3 -O0 -fstack-protector -fno-omit-frame-pointer -pie -fPIE -Werror=vla")

set(CMAKE_C_FLAGS_RELEASE "-O2")

if(ENABLE_SANITIZERS)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}                             \
        -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow       \
        -fsanitize=float-divide-by-zero,integer-divide-by-zero,leak             \
        -fsanitize=nonnull-attribute,null,object-size,return,shift              \
        -fsanitize=signed-integer-overflow,undefined,unreachable,vla-bound"
    )
endif()

set(SOURCES src/shm_main.c)
set(HEADERS include/common.h include/shm.h)

add_executable(shm_run ${SOURCES} ${HEADERS})

target_include_directories(shm_run PRIVATE include)
