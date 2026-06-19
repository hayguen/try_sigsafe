
if ( CMAKE_C_COMPILER_ID MATCHES "MSVC" )
    # using Visual Studio C++
    message(STATUS "INFO: detected MSVC: will not link math lib m")
    set(MATHLIB "")

    add_definitions("/D_CRT_SECURE_NO_WARNINGS")
    set(MSVC_DISABLED_WARNINGS_LIST "C4996")
else()
    if(DISABLE_LINK_WITH_M)
    else()
        message(STATUS "INFO: detected NO MSVC: ${CMAKE_C_COMPILER_ID}: will link math lib m")
        set(MATHLIB "m")
    endif()
endif()

add_library( try_sigsafe_pp STATIC try_sigsafe_pp.cpp try_sigsafe_pp.h )
target_link_libraries( try_sigsafe_pp ${MATHLIB} )
target_include_directories( try_sigsafe_pp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET try_sigsafe_pp PROPERTY CXX_STANDARD 11)
set_property(TARGET try_sigsafe_pp PROPERTY CXX_STANDARD_REQUIRED ON)

