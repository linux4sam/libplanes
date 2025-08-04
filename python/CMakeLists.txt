include(UseSWIG)

swig_add_library(pyplanes
    TYPE MODULE
    LANGUAGE python
    SOURCES ${CMAKE_SOURCE_DIR}/python/planes.i
)

set_target_properties(pyplanes
    PROPERTIES
        SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE
        SWIG_COMPILE_OPTIONS -Wall
        OUTPUT_NAME "planes"
)

target_include_directories(pyplanes
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${Python3_INCLUDE_DIRS}
        ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(pyplanes
    PRIVATE
        ${Python3_LIBRARIES_DIRS}
        ${LIBDRM_LIBRARIES_DIRS}
)

target_link_libraries(pyplanes
    PRIVATE
        planes
        ${Python3_LIBRARIES}
        ${LIBDRM_LIBRARIES}
)

# Don't request the site-packages path to Python. It doesn't work in a
# cross-compilation environment.
set(PYTHON_VERSION "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
set(PYTHON_SITE_PKG
    "${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION}/site\-packages"
)

install(TARGETS pyplanes
    LIBRARY 
        DESTINATION ${PYTHON_SITE_PKG}/planes
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/planes.py
    ${CMAKE_SOURCE_DIR}/python/__init__.py
    DESTINATION ${PYTHON_SITE_PKG}/planes
)

