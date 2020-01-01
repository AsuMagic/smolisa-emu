cmake_minimum_required(VERSION 2.8)

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

# Emulator
add_executable(smolisa-emu ${SOURCES_COMMON} ${SOURCES_EMULATOR})

target_compile_features(smolisa-emu PRIVATE cxx_std_17)
target_include_directories(smolisa-emu PRIVATE "src/")

target_link_libraries(smolisa-emu PRIVATE
    fmt
    sfml-window sfml-graphics sfml-system # TODO: make SFML optional
)

# Assembler
add_executable(smolisa-as ${SOURCES_COMMON} ${SOURCES_ASSEMBLER})

target_compile_features(smolisa-as PRIVATE cxx_std_17)
target_include_directories(smolisa-as PRIVATE "src/")

target_link_libraries(smolisa-as PRIVATE fmt)

# Video stream generator
add_executable(example-generate-video-stream "examples/generate_video_stream.cpp")

target_compile_features(smolisa-as PRIVATE cxx_std_17)
