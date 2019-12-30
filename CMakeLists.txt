cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(smolisa-emu)
add_executable(${PROJECT_NAME}
	"main.cpp"
	"core.cpp"
	"memory.cpp"
)

target_link_libraries(${PROJECT_NAME} PRIVATE fmt)
