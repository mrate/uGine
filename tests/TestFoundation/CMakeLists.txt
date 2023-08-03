add_executable(
	TestFoundation
		main.cpp
		TestAllocators.cpp
		TestBasic.cpp
		TestCollections.cpp
		TestDelegates.cpp
		TestGlm.cpp
		TestImage.cpp
		TestOs.cpp
		TestPath.cpp
		TestSerialization.cpp
		TestStrings.cpp
)

target_link_libraries(
	TestFoundation
		uGine::Foundation
		gtest
)

add_test(
	NAME TestFoundation
	COMMAND TestFoundation
)