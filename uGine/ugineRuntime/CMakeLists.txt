project(uGineRuntime)

add_executable(
	${PROJECT_NAME}
		source/runtime.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		uGine::uGine
)
