cmake_minimum_required(VERSION 3.24)

project(vertify)

add_executable(
	vertify
		src/main.cpp
)

target_link_libraries(
	vertify
		assimp::assimp
)

target_compile_definitions(
	vertify
	PUBLIC
		${UGINE_COMPILE_DEFINITIONS}
)
