cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(smolisa-emu)
add_executable(${PROJECT_NAME}
        "src/smol/emu/main.cpp"
	"src/smol/emu/core.cpp"
	"src/smol/emu/memory.cpp"
)

target_include_directories(${PROJECT_NAME} PRIVATE "src/")

target_link_libraries(${PROJECT_NAME} PRIVATE fmt)
