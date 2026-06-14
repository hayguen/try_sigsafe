
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

add_library( try_sigsafe STATIC try_sigsafe.c try_sigsafe.h )
target_link_libraries( try_sigsafe ${MATHLIB} )
target_include_directories( try_sigsafe INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET try_sigsafe PROPERTY C_STANDARD 11)
set_property(TARGET try_sigsafe PROPERTY C_STANDARD_REQUIRED ON)
# set_property(TARGET try_sigsafe PROPERTY C_EXTENSIONS OFF)
