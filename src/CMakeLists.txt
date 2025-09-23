add_library(planes SHARED
    common.c
    drm-object.c
    fb.c
    kms-crtc.c
    kms-device.c
    kms-framebuffer.c
    kms-plane.c
    kms-screen.c
    plane.c
)

set_target_properties(planes PROPERTIES VERSION 3.0.0 SOVERSION 3)

target_sources(planes
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/include/
        FILES
            ${CMAKE_SOURCE_DIR}/include/planes/fb.h
            ${CMAKE_SOURCE_DIR}/include/planes/kms.h
            ${CMAKE_SOURCE_DIR}/include/planes/plane.h
)

target_include_directories(planes PRIVATE ${LIBDRM_INCLUDE_DIRS})
target_link_directories(planes PRIVATE ${LIBDRM_LIBRARIES_DIRS})
target_link_libraries(planes PRIVATE ${LIBDRM_LIBRARIES})

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(libdir \${exec_prefix}/lib)
set(includedir \${prefix}/include)
set(PACKAGE_NAME libplanes)
set(PACKAGE_VERSION ${PROJECT_VERSION})

configure_file(${CMAKE_SOURCE_DIR}/libplanes.pc.in ${CMAKE_SOURCE_DIR}/libplanes.pc @ONLY)

if(ENABLE_ENGINE)
    target_sources(planes
        PRIVATE
            draw.c
            engine.c
            script.c
        PUBLIC
            FILE_SET HEADERS
            FILES
                ${CMAKE_SOURCE_DIR}/include/planes/draw.h
                ${CMAKE_SOURCE_DIR}/include/planes/engine.h
    )

    target_include_directories(planes
        PRIVATE
            ${CAIRO_INCLUDE_DIRS}
            ${CJSON_INCLUDE_DIRS}
            ${LUA_INCLUDE_DIRS}
    )

    target_link_directories(planes
        PRIVATE
            ${CAIRO_LIBRARIES_DIRS}
            ${CJSON_LIBRARIES_DIRS}
            ${LUA_LIBARIES_DIRS}
    )

    target_link_libraries(planes
        PRIVATE
            ${CAIRO_LIBRARIES}
            ${CJSON_LIBRARIES}
            ${LUA_LIBRARIES}
    )
endif()

install(TARGETS planes
    LIBRARY
    FILE_SET HEADERS
)

install(FILES ${CMAKE_SOURCE_DIR}/libplanes.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

