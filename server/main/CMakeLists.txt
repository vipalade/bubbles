# the bubble service


# openssl genrsa 2048 > ca-key.pem
# openssl req -new -x509 -nodes -days 1000 -key ca-key.pem > ca-cert.pem
# openssl req -newkey rsa:2048 -days 1000 -nodes -keyout server-key.pem > server-req.pem
# openssl x509 -req -in server-req.pem -days 1000 -CA ca-cert.pem -CAkey ca-key.pem -set_serial 01 > server-cert.pem
# openssl req -newkey rsa:2048 -days 1000 -nodes -keyout client-key.pem > client-req.pem
# openssl x509 -req -in client-req.pem -days 1000 -CA ca-cert.pem -CAkey ca-key.pem -set_serial 01 > client-cert.pem

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bubbles-ca-cert.pem
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/certs/bubbles-ca-cert.pem ${CMAKE_CURRENT_BINARY_DIR}/bubbles-ca-cert.pem
    DEPENDS ${PROJECT_SOURCE_DIR}/certs/bubbles-ca-cert.pem
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bubbles-server-key.pem
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/certs/bubbles-server-key.pem ${CMAKE_CURRENT_BINARY_DIR}/bubbles-server-key.pem
    DEPENDS ${PROJECT_SOURCE_DIR}/certs/bubbles-server-key.pem
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bubbles-server-cert.pem
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/certs/bubbles-server-cert.pem ${CMAKE_CURRENT_BINARY_DIR}/bubbles-server-cert.pem
    DEPENDS ${PROJECT_SOURCE_DIR}/certs/bubbles-server-cert.pem
)

add_custom_target(bubbles_server_certs
    DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/bubbles-ca-cert.pem
    ${CMAKE_CURRENT_BINARY_DIR}/bubbles-server-key.pem
    ${CMAKE_CURRENT_BINARY_DIR}/bubbles-server-cert.pem
)

add_executable (bubbles_server src/bubbles_server_main.cpp src/bubbles_server_engine.cpp ../../protocol/bubbles_messages.hpp)

add_dependencies(bubbles_server bubbles_server_certs build_snappy)

target_link_libraries (bubbles_server
    solid_frame_mpipc
    solid_frame_aio_openssl
    solid_serialization_v2
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${SNAPPY_LIB}
    ${SYS_BASIC_LIBS}
    ${SYS_DYNAMIC_LOAD_LIB}
)

