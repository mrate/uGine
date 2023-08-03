add_executable(
	JobTest
		src/main.cpp
		src/threadTest.cpp
		src/jobTest.cpp
)

target_link_libraries(
	JobTest
		uGine::Foundation
)