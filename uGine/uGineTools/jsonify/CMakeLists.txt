cmake_minimum_required(VERSION 3.24)

project(jsonify)

add_executable(
	jsonify
		src/main.cpp
)

target_compile_definitions(
	jsonify
	PUBLIC
		${UGINE_COMPILE_DEFINITIONS}
)
