cmake_minimum_required(VERSION 3.9)

project(smolisa-tools)

option(OPTION_FRAMEBUFFER "Emulate the MMIO framebuffer (requires SFML)" ON)

function(component target)
	target_compile_features(${target} PRIVATE cxx_std_20)
	target_include_directories(${target} PRIVATE "include/")
	target_compile_definitions(${target} PRIVATE $<$<BOOL:${OPTION_FRAMEBUFFER}>:SMOLISA_FRAMEBUFFER>)
endfunction()

set(SOURCES_COMMON
	"src/smol/common/ioutil.cpp"
)

set(SOURCES_EMULATOR_FRAMEBUFFER
	"src/smol/emu/framebuffer/framebuffer.cpp"
)

set(SOURCES_EMULATOR
	"src/smol/emu/main.cpp"
	"src/smol/emu/core.cpp"
	"src/smol/emu/memory.cpp"
	$<$<BOOL:${OPTION_FRAMEBUFFER}>:${SOURCES_EMULATOR_FRAMEBUFFER}>
)

set(SOURCES_ASSEMBLER
	"src/smol/asm/assembler.cpp"
	"src/smol/asm/main.cpp"
	"src/smol/asm/tokenizer.cpp"
)

set(LIBRARY_SFML "sfml-system" "sfml-window" "sfml-graphics")

# Common
add_library(smolisa-common STATIC ${SOURCES_COMMON})
component(smolisa-common)

# Emulator
add_executable(smolisa-emu ${SOURCES_EMULATOR})
component(smolisa-emu)

target_link_libraries(smolisa-emu PRIVATE
    smolisa-common
	fmt
)

if (${OPTION_FRAMEBUFFER} STREQUAL ON)
	target_link_libraries(smolisa-emu PRIVATE ${LIBRARY_SFML})
endif()

# Assembler
add_executable(smolisa-as ${SOURCES_ASSEMBLER})
component(smolisa-as)

target_link_libraries(smolisa-as PRIVATE smolisa-common fmt)

# Video stream generator
add_executable(example-generate-video-stream "examples/generate_video_stream.cpp")
component(example-generate-video-stream)
