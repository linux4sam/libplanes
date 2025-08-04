if(ENABLE_ENGINE)
    add_executable(planes_engine planes.c)

    set_target_properties(planes_engine
        PROPERTIES
            OUTPUT_NAME "planes"
    )

    target_include_directories(planes_engine
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${LIBDRM_INCLUDE_DIRS}
    )

    target_link_directories(planes_engine
        PRIVATE
            ${LIBDRM_LIBRARIES_DIRS}
    )

    target_link_libraries(planes_engine
        PRIVATE
            planes
            ${LIBDRM_LIBRARIES}
    )

    add_executable(grab grab.c)

    target_include_directories(grab
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CAIRO_INCLUDE_DIRS}
            ${LIBDRM_INCLUDE_DIRS}
    )

    target_link_directories(grab
        PRIVATE
            ${CAIRO_LIBRARIES_DIRS}
            ${LIBDRM_LIBRARIES_DIRS}
    )

    target_link_libraries(grab
        PRIVATE
            planes
            ${CAIRO_LIBRARIES}
            ${LIBDRM_LIBRARIES}
    )

    add_executable(render render.c)

    target_include_directories(render
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CAIRO_INCLUDE_DIRS}
            ${LIBDRM_INCLUDE_DIRS}
    )

    target_link_directories(render
        PRIVATE
            ${CAIRO_LIBRARIES_DIRS}
            ${LIBDRM_LIBRARIES_DIRS}
    )

    target_link_libraries(render
        PRIVATE
            planes
            ${CAIRO_LIBRARIES}
            ${LIBDRM_LIBRARIES}
    )

    install(TARGETS planes_engine grab render RUNTIME)
endif()

if(DIRECTFB_FOUND)
    add_executable(dfblayers dfblayers.c)
    target_include_directories(dfblayers PRIVATE ${DIRECTFB_INCLUDE_DIRS})
    target_link_directories(dfblayers PRIVATE ${DIRECTFB_LIBRARIES_DIRS})
    target_link_libraries(dfblayers PRIVATE libplanes ${DIRECTFB_LIBRARIES})
    install(TARGETS dfblayers RUNTIME)
endif()

