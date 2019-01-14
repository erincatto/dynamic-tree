find_package(Threads REQUIRED)

if (UNIX AND NOT APPLE)
    find_library(RT_LIBRARY rt)
    if (RT_LIBRARY)
        list(APPEND glfw_LIBRARIES "${RT_LIBRARY}")
    endif()

    find_library(MATH_LIBRARY m)
    if (MATH_LIBRARY)
        list(APPEND glfw_LIBRARIES "${MATH_LIBRARY}")
    endif()

    if (CMAKE_DL_LIBS)
        list(APPEND glfw_LIBRARIES "${CMAKE_DL_LIBS}")
    endif()

    find_package(X11 REQUIRED)

    # Set up library and include paths
    list(APPEND glfw_INCLUDE_DIRS "${X11_X11_INCLUDE_PATH}")
    list(APPEND glfw_LIBRARIES "${X11_X11_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

    # Check for XRandR (modern resolution switching and gamma control)
    if (NOT X11_Xrandr_FOUND)
        message(FATAL_ERROR "The RandR headers were not found")
    endif()

    # Check for Xinerama (legacy multi-monitor support)
    if (NOT X11_Xinerama_FOUND)
        message(FATAL_ERROR "The Xinerama headers were not found")
    endif()

    # Check for Xkb (X keyboard extension)
    if (NOT X11_Xkb_FOUND)
        message(FATAL_ERROR "The X keyboard extension headers were not found")
    endif()

    # Check for Xcursor (cursor creation from RGBA images)
    if (NOT X11_Xcursor_FOUND)
        message(FATAL_ERROR "The Xcursor headers were not found")
    endif()

    list(APPEND glfw_INCLUDE_DIRS "${X11_Xrandr_INCLUDE_PATH}"
                                  "${X11_Xinerama_INCLUDE_PATH}"
                                  "${X11_Xkb_INCLUDE_PATH}"
                                  "${X11_Xcursor_INCLUDE_PATH}")
endif()

if (APPLE)
    list(APPEND glfw_LIBRARIES
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework CoreVideo")
endif()

set(common_HEADERS internal.h mappings.h
                   ../include/GLFW/glfw3.h
                   ../include/GLFW/glfw3native.h)
set(common_SOURCES context.c init.c input.c monitor.c vulkan.c window.c)

if (APPLE)
    set(glfw_HEADERS ${common_HEADERS} cocoa_platform.h cocoa_joystick.h
                     posix_thread.h nsgl_context.h egl_context.h osmesa_context.h)
    set(glfw_SOURCES ${common_SOURCES} cocoa_init.m cocoa_joystick.m
                     cocoa_monitor.m cocoa_window.m cocoa_time.c posix_thread.c
                     nsgl_context.m egl_context.c osmesa_context.c)
    set_source_files_properties(${glfw_SOURCES} PROPERTIES LANGUAGE C)
elseif (WIN32)
    set(glfw_HEADERS ${common_HEADERS} win32_platform.h win32_joystick.h
                     wgl_context.h egl_context.h osmesa_context.h)
    set(glfw_SOURCES ${common_SOURCES} win32_init.c win32_joystick.c
                     win32_monitor.c win32_time.c win32_thread.c win32_window.c
                     wgl_context.c egl_context.c osmesa_context.c)
elseif (UNIX)
    set(glfw_HEADERS ${common_HEADERS} x11_platform.h xkb_unicode.h posix_time.h
                     posix_thread.h glx_context.h egl_context.h osmesa_context.h)
    set(glfw_SOURCES ${common_SOURCES} x11_init.c x11_monitor.c x11_window.c
                     xkb_unicode.c posix_time.c posix_thread.c glx_context.c
                     egl_context.c osmesa_context.c)
    set(glfw_HEADERS ${glfw_HEADERS} linux_joystick.h)
    set(glfw_SOURCES ${glfw_SOURCES} linux_joystick.c)
endif()

add_library(glfw ${glfw_SOURCES} ${glfw_HEADERS})
target_include_directories(glfw PUBLIC ../include)
target_include_directories(glfw PRIVATE ${glfw_INCLUDE_DIRS})
target_link_libraries(glfw INTERFACE ${glfw_LIBRARIES})
