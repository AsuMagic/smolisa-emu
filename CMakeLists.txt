cmake_minimum_required(VERSION 3.9)

project(smolisa-emu)

option(OPTION_FRAMEBUFFER "Emulate the MMIO framebuffer (requires SFML)" ON)

function(component target)
	target_compile_features(${target} PRIVATE cxx_std_20)
	target_include_directories(${target} PRIVATE "include/")
	target_compile_definitions(${target} PRIVATE $<$<BOOL:${OPTION_FRAMEBUFFER}>:SMOLISA_FRAMEBUFFER>)
endfunction()

set(SOURCES_EMULATOR_FRAMEBUFFER
		"src/framebuffer/framebuffer.cpp"
)

set(SOURCES_EMULATOR
	"src/ioutil.cpp"
	"src/main.cpp"
	"src/core.cpp"
	"src/memory.cpp"
	$<$<BOOL:${OPTION_FRAMEBUFFER}>:${SOURCES_EMULATOR_FRAMEBUFFER}>
)

add_executable(smolisa-emu ${SOURCES_EMULATOR})
component(smolisa-emu)

target_link_libraries(smolisa-emu PRIVATE fmt)

if (${OPTION_FRAMEBUFFER} STREQUAL ON)
		target_link_libraries(smolisa-emu PRIVATE "sfml-system" "sfml-window" "sfml-graphics")
endif()

# Video stream generator
add_executable(example-generate-video-stream "smol2/examples/generate_video_stream.cpp")
component(example-generate-video-stream)
