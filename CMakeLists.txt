cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(wxGrabber)

if(MSVC)
    add_compile_options(/std:c++17 /Zc:__cplusplus)
endif()

set(wxWidgets_ROOT_DIR "C:/Programs/WxWidgets/wxWidgets")
set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc_x64_dll")

# Find wxWidgets with media component
find_package(wxWidgets COMPONENTS core base adv media REQUIRED)
include(${wxWidgets_USE_FILE})

# Add compiler definitions to handle C/C++ interop
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

file(GLOB_RECURSE project_GLOB
    src/*.cpp
    #src/*.c
)

# print project_GLOB
message(STATUS "project_GLOB: ${project_GLOB}")

add_executable(wxGrabber ${project_GLOB})

target_link_libraries(wxGrabber PRIVATE ${wxWidgets_LIBRARIES})
target_include_directories(wxGrabber PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_include_directories(wxGrabber PRIVATE include)

# minivorbis c implementation
add_library(minivorbis_impl src/vorbis/minivorbis_impl.c)
target_compile_options(minivorbis_impl PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/w>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
)
target_link_libraries(wxGrabber PRIVATE minivorbis_impl)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    file(GLOB dlls_GLOB
        ${wxWidgets_LIB_DIR}/*ud*.dll
    )
else()
    file(GLOB dlls_GLOB
        ${wxWidgets_LIB_DIR}/*.dll
    )
    file(GLOB dllsDebug_GLOB
        ${wxWidgets_LIB_DIR}/*ud*.dll
    )
    list(REMOVE_ITEM dlls_GLOB ${dllsDebug_GLOB})
endif()

add_custom_command(TARGET wxGrabber POST_BUILD          # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_if_different           # which executes "cmake - E copy_if_different..."
        ${dlls_GLOB}                                        # <--this is in-file
        $<TARGET_FILE_DIR:wxGrabber>)                   # <--this is out-file path