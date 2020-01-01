cmake_minimum_required(VERSION 3.9)

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

function(component target)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE "include/")
endfunction()

project(smolisa-tools)

# Common
add_library(smolisa-common STATIC ${SOURCES_COMMON})
component(smolisa-common)

# Emulator
add_executable(smolisa-emu ${SOURCES_EMULATOR})
component(smolisa-emu)

target_link_libraries(smolisa-emu PRIVATE
    smolisa-common
    fmt
    sfml-window sfml-graphics sfml-system # TODO: make SFML optional
)

# Assembler
add_executable(smolisa-as ${SOURCES_ASSEMBLER})
component(smolisa-as)

target_link_libraries(smolisa-as PRIVATE smolisa-common fmt)

# Video stream generator
add_executable(example-generate-video-stream "examples/generate_video_stream.cpp")
component(example-generate-video-stream)
