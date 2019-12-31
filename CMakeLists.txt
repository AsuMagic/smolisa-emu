cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES_COMMON
    "src/smol/common/ioutil.cpp"
)

set(SOURCES_EMULATOR
    "src/smol/emu/main.cpp"
    "src/smol/emu/core.cpp"
    "src/smol/emu/memory.cpp"
    "src/smol/emu/framebuffer/framebuffer.cpp" # TODO: make this optional
)

set(SOURCES_ASSEMBLER
    "src/smol/asm/assembler.cpp"
    "src/smol/asm/main.cpp"
    "src/smol/asm/tokenizer.cpp"
)

project(smolisa-tools)

add_executable(smolisa-emu ${SOURCES_COMMON} ${SOURCES_EMULATOR})

target_include_directories(smolisa-emu PRIVATE "src/")

target_link_libraries(smolisa-emu PRIVATE
    fmt
    sfml-window sfml-graphics sfml-system # TODO: make SFML optional
)

add_executable(smolisa-as ${SOURCES_COMMON} ${SOURCES_ASSEMBLER})

target_include_directories(smolisa-as PRIVATE "src/")

target_link_libraries(smolisa-as PRIVATE fmt)
