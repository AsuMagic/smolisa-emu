cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(smolisa-tools)

add_executable(smolisa-emu
    "src/smol/emu/main.cpp"
    "src/smol/emu/core.cpp"
    "src/smol/emu/memory.cpp"
    "src/smol/emu/framebuffer/framebuffer.cpp" # TODO: make this optional
)

target_include_directories(smolisa-emu PRIVATE "src/")

target_link_libraries(smolisa-emu PRIVATE
    fmt
    sfml-window sfml-graphics sfml-system # TODO: make SFML optional
)

add_executable(smolisa-as
    "src/smol/asm/main.cpp"
)

target_include_directories(smolisa-as PRIVATE "src/")

target_link_libraries(smolisa-as PRIVATE fmt)
