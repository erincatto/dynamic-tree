project(samples LANGUAGES CXX)

set (SAMPLE_SOURCE_FILES
	glad.c
	imgui_impl_glfw.cpp
	imgui_impl_opengl3.cpp
	draw.cpp
	main.cpp
	test1.cpp)

set (SAMPLE_HEADER_FILES
	khrplatform.h
	glad.h
	imgui_impl_glfw.h
	imgui_impl_opengl3.h
	draw.h
	test.h)

add_executable(samples ${SAMPLE_SOURCE_FILES} ${SAMPLE_HEADER_FILES})
target_include_directories(samples PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(samples PUBLIC dynamic-tree glfw imgui ${OPENGL_LIBRARIES})
