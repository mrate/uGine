add_executable(
	ScriptTest
		source/main.cpp
)

target_link_libraries(
	ScriptTest
		libluajit
		LuaBridge
		
		uGine::uGine
)