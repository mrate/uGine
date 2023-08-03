find_package(Vulkan REQUIRED)

add_library(
	uGineFoundation
	STATIC
		ugine/Align.h
		ugine/Easing.h
		ugine/ArrayProxy.h		
		
		ugine/Concurrent.h
		#ugine/Compression.cpp
		#ugine/Compression.h
		ugine/Color.cpp
		ugine/Color.h
		ugine/EventEmittor.h
		ugine/Error.h
		ugine/Fiber.cpp
		ugine/Fiber.h
		ugine/File.cpp
		ugine/File.h
		ugine/FileWatcher.h
		ugine/FileSystem.cpp
		ugine/FileSystem.h
		ugine/FpsCounter.h
		ugine/Hash.cpp
		ugine/Hash.h
		ugine/Image.cpp
		ugine/Image.h
		ugine/Jobs.cpp
		ugine/Jobs.h
		ugine/Locking.cpp
		ugine/Locking.h
		ugine/Log.cpp
		ugine/Log.h
		ugine/Memory.cpp
		ugine/Memory.h
		ugine/Os.cpp
		ugine/Os.h
		ugine/Path.cpp
		ugine/Path.h
		ugine/Permutations.h
		ugine/Profile.cpp
		ugine/Profile.h
		ugine/Serialization.h		
		ugine/Scheduler.cpp
		ugine/Scheduler.h
		ugine/Span.h
		ugine/SlotMap.h					
		ugine/Stream.cpp
		ugine/Stream.h
		ugine/String.cpp
		ugine/String.h
		ugine/StringUtils.cpp
		ugine/StringUtils.h
		ugine/StackTrace.cpp
		ugine/StackTrace.h
		ugine/TypeContainer.h
		ugine/Thread.cpp
		ugine/Thread.h
		ugine/UniqueType.h
		ugine/Utils.h
		ugine/Ugine.cpp
		ugine/Ugine.h
		ugine/UUID.cpp
		ugine/UUID.h
		ugine/Vector.h
)

if (MSVC)
	target_sources(
		uGineFoundation
		PUBLIC
			vs/Path.natvis
			vs/SlotMap.natvis
			vs/Span.natvis
			vs/String.natvis
			vs/UniquePtr.natvis
			vs/Vector.natvis
	)
endif ()

add_library(uGine::Foundation ALIAS uGineFoundation)

FIND_PACKAGE(ZLIB)

target_include_directories(
	uGineFoundation
	PUBLIC
		./
)

target_link_libraries(
	uGineFoundation
	PUBLIC
		uGine::CompilerFlags
		
		Bitsery::bitsery
		enkiTS::enkiTS
		EnTT::EnTT
		FileWatch
		glm::glm
		nlohmann_json
		spdlog::spdlog
)

target_link_libraries(
	uGineFoundation
	PRIVATE
		FileWatch
		ktx
		mimalloc-static
		stb
		tlsf
		Vulkan::Headers
)

if (WIN32)
	target_link_libraries(
		uGineFoundation
		PRIVATE
			DbgHelp
	)
endif()

if (UGINE_TRACE_ALLOCATIONS)
	target_compile_definitions(
		uGineFoundation
		PUBLIC
			UGINE_TRACE_ALLOCATIONS
	)
endif ()

if (UGINE_TRACE_ALLOCATIONS_CNT)
	target_compile_definitions(
		uGineFoundation
		PUBLIC
			UGINE_TRACE_ALLOCATIONS_CNT
	)
endif ()

if (UGINE_PROFILE)
	if (TRACY_ENABLE)
		target_link_libraries(
			uGineFoundation
			PUBLIC
				Tracy::TracyClient
		)

		target_compile_definitions(
			uGineFoundation
			PUBLIC
				UGINE_PROFILE
				UGINE_PROFILE_TRACY
				TRACY_FIBERS
	
		)
	elseif (OPTICK_ENABLE)
		target_link_libraries(
			uGineFoundation
			PUBLIC
				OptickCore
		)

		target_compile_definitions(
			uGineFoundation
			PUBLIC
				UGINE_PROFILE
				UGINE_PROFILE_OPTICK
	
		)
	endif()
endif()