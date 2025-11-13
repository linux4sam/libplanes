cmake_minimum_required(VERSION 3.23)

project(libplanes
    VERSION 2.0.0
    DESCRIPTION "Microchip hardware LCD plane library"
    LANGUAGES C
)
#set(PROJECT_VERSION_RC 1)
if(DEFINED PROJECT_VERSION_RC)
    string(APPEND PROJECT_VERSION "-rc${PROJECT_VERSION_RC}")
endif()

include(GNUInstallDirs)

set(CMAKE_VERBOSE_MAKEFILE True)

add_compile_options(-Wall -Wextra)
add_compile_options(-Wsign-compare)
add_compile_options(-Werror-implicit-function-declaration)
add_compile_options(-Wpointer-arith)
add_compile_options(-Wwrite-strings)
add_compile_options(-Wstrict-prototypes)
add_compile_options(-Wmissing-prototypes -Wmissing-declarations)
add_compile_options(-Wnested-externs)
add_compile_options(-Wpacked)
add_compile_options(-Wswitch-enum)
add_compile_options(-Wstrict-aliasing=2)
add_compile_options(-Winit-self)
add_compile_options(-Wdeclaration-after-statement)
add_compile_options(-Wold-style-definition)
add_compile_options(-Wno-unused-parameter)
add_compile_options(-Wno-attributes)
add_compile_options(-Wno-long-long)
add_compile_options(-Winline)
add_compile_options(-Wshadow)
add_compile_options(-Wno-missing-field-initializers)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBDRM REQUIRED libdrm>=2.4.0)
set(AX_PACKAGE_REQUIRES "libdrm >= 2.4.0")

pkg_check_modules(DIRECTFB directfb>=1.7.0)

option(ENABLE_ENGINE "enable/disable engine [default=OFF]" OFF)
if(ENABLE_ENGINE)
    pkg_check_modules(CAIRO REQUIRED cairo>=1.14.6)
    set(AX_PACKAGE_REQUIRES_PRIVATE "cairo >= 1.14.6")

    pkg_check_modules(CJSON REQUIRED libcjson>=1.6.0)
    set(AX_PACKAGE_REQUIRES_PRIVATE "${AX_PACKAGE_REQUIRES_PRIVATE} libcjson >= 1.6.0")

    pkg_search_module(LUA REQUIRED lua>=5.3.1 lua5.3)
    set(AX_PACKAGE_REQUIRES_PRIVATE "${AX_PACKAGE_REQUIRES_PRIVATE} lua >= 5.3.1")

    find_package(Python3 COMPONENTS Interpreter Development)
    if(Python3_FOUND)
        find_package(SWIG REQUIRED COMPONENTS python)
    endif()
endif()

add_subdirectory(src)

if(ENABLE_ENGINE OR DIRECTFB_FOUND)
    add_subdirectory(apps)
endif()

if(Python3_FOUND AND SWIG_FOUND)
    add_subdirectory(python)
endif()
