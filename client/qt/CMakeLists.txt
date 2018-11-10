set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# qt5_wrap_ui(
#   uic_basic_gui
#   main_window.ui
# )

# qt5_add_resources(
#   res
#   icons.qrc
# )


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bubbles-ca-cert.pem
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/certs/bubbles-ca-cert.pem ${CMAKE_CURRENT_BINARY_DIR}/bubbles-ca-cert.pem
    DEPENDS ${PROJECT_SOURCE_DIR}/certs/bubbles-ca-cert.pem
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bubbles-client-key.pem
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/certs/bubbles-client-key.pem ${CMAKE_CURRENT_BINARY_DIR}/bubbles-client-key.pem
    DEPENDS ${PROJECT_SOURCE_DIR}/certs/bubbles-client-key.pem
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bubbles-client-cert.pem
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/certs/bubbles-client-cert.pem ${CMAKE_CURRENT_BINARY_DIR}/bubbles-client-cert.pem
    DEPENDS ${PROJECT_SOURCE_DIR}/certs/bubbles-client-cert.pem
)

add_custom_target(bubbles_client_certs
    DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/bubbles-ca-cert.pem
    ${CMAKE_CURRENT_BINARY_DIR}/bubbles-client-key.pem
    ${CMAKE_CURRENT_BINARY_DIR}/bubbles-client-cert.pem
)

list(APPEND CMAKE_PREFIX_PATH "${EXTERNAL_PATH}")
find_package(Qt5Widgets)

if(${Qt5Widgets_FOUND} AND Boost_FOUND)
    set(
        src_basic_gui
        src/bubbles_client_main.cpp
        src/bubbles_client_widget.hpp
        src/bubbles_client_widget.cpp
    )

    if(WIN32)  
        add_executable(
            bubbles_client
            WIN32
            ${uic_basic_gui}
        #   ${res}
            ${src_basic_gui}
        )
    elseif(APPLE)
        add_executable(
            bubbles_client
            MACOSX_BUNDLE
            ${uic_basic_gui}
        #   ${res}
            ${src_basic_gui}
        )
    else()
        add_executable(
            bubbles_client
            ${uic_basic_gui}
        #   ${res}
            ${src_basic_gui}
        )
    endif()

    add_dependencies(bubbles_client bubbles_client_certs build_snappy)

    target_link_libraries(
        bubbles_client
        bubbles_client_engine
        solid_frame_mprpc
        solid_frame_aio_openssl
        solid_serialization_v2
        ${OPENSSL_LIBRARIES}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        Qt5::Widgets
        ${SNAPPY_LIB}
        ${SYS_BASIC_LIBS}
        ${SYS_DYNAMIC_LOAD_LIB}
    )
endif()
