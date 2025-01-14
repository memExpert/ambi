cmake_minimum_required(VERSION 3.25)
project(ambi LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(USE_HW_ACCELERATION "Enable hardware acceleration (OpenGL or Vulkan)" ON)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(LIB_OPTPARSE "${LIB_DIR}/optparse")

file(GLOB SRC_FILES "${SRC_DIR}/*.c")
file(GLOB INCLUDE_FILES "${INCLUDE_DIR}/*.h")

find_package(X11 REQUIRED)

add_executable(ambi ${SRC_FILES})

target_include_directories(ambi PRIVATE ${INCLUDE_DIR} ${LIB_OPTPARSE})
target_link_libraries(ambi PRIVATE ${X11_LIBRARIES} Xext)


if(USE_HW_ACCELERATION)
    find_package(OpenGL REQUIRED)
    target_link_libraries(ambi PRIVATE OpenGL::GL)
    message(STATUS "Hardware acceleration enabled: Using OpenGL")
else()
    message(STATUS "Hardware acceleration disabled")
endif()



if(WIN32)
    target_compile_definitions(ambi PRIVATE -DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(ambi PRIVATE -DPLATFORM_LINUX)
endif()



#if(CMAKE_BUILD_TYPE STREQUAL "Release")
#    target_compile_options(ambi PRIVATE -O3)
#elseif((CMAKE_BUILD_TYPE STREQUAL "Debug"))
#endif()



#enable_testing()
#add_subdirectory(tests)