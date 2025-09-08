cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_FLAGS "                                                                                     \
    -D_DEBUG -ggdb3 -O0 -Wall -Wextra                                                                   \
    -Wcast-align -Wcast-qual -Wchar-subscripts -Wconversion -Wempty-body -Wfloat-equal                  \
    -Wformat-security -Wformat=2 -Wlogical-op -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow    \
    -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused                                  \
    -Wno-missing-field-initializers -Wno-narrowing -Wstack-protector                                    \
    -fstack-protector -fno-omit-frame-pointer -pie -fPIE -Werror=vla                                    \
    -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,shift,signed-integer-overflow,undefined,unreachable,vla-bound \
")

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
