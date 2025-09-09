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

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_executable(cmd_emulator ${SOURCES} ${HEADERS})

target_include_directories(
    cmd_emulator
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cmd_emulator)

# cmake -B build -DCMAKE_BUILD_TYPE=Debug
# cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
# cmake -B build -DCMAKE_BUILD_TYPE=Release
