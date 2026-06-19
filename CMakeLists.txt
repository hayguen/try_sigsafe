cmake_minimum_required(VERSION 3.5)
project(TRY_SIGSAFE)

# # C90 requires the gcc extensions for function attributes like always_inline
# # C99 provides the function attributes: no gcc extensions required
# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_EXTENSIONS OFF)
#
# set(CMAKE_CXX_STANDARD 98)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)


########################################################################
# select the release build type by default to get optimization flags
########################################################################
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)


add_subdirectory( try_sigsafe )
add_subdirectory( try_sigsafe_pp )


if ( CMAKE_C_COMPILER_ID MATCHES "MSVC" )
    # using Visual Studio C++
    add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif()


add_executable( test_c test_c.c )
add_executable( test_cpp test.cpp )

target_link_libraries( test_c   try_sigsafe )
target_link_libraries( test_cpp try_sigsafe_pp )

set_property(TARGET test_c   PROPERTY C_STANDARD 11)
set_property(TARGET test_c   PROPERTY C_STANDARD_REQUIRED ON)

set_property(TARGET test_cpp PROPERTY CXX_STANDARD 11)
set_property(TARGET test_cpp PROPERTY CXX_STANDARD_REQUIRED ON)


enable_testing()

add_test(NAME test_c
  COMMAND "${CMAKE_CURRENT_BINARY_DIR}/test_c"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
