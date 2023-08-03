add_executable(
	MaterialTest
		main.cpp
)

target_link_libraries(
	MaterialTest	
		uGine::uGine
		uGine::materialBuilder
		uGine::materialImporter
)