cmake_minimum_required(VERSION 3.8)

project(dynamic-tree LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

add_subdirectory(src)

option(BUILD_SAMPLES "Build the dynamic-tree sample program" ON)

if (BUILD_SAMPLES)

	find_package(OpenGL REQUIRED)

	add_subdirectory(extern/glfw)
	add_subdirectory(extern/imgui)
	add_subdirectory(extern/sajson)
	add_subdirectory(samples)

	# default startup project for Visual Studio
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT samples)
		set_property(TARGET samples PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/samples")
	endif()

endif()
