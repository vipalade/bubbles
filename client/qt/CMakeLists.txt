set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# qt5_wrap_ui(
# 	uic_basic_gui
# 	main_window.ui
# )

# qt5_add_resources(
# 	res
# 	icons.qrc
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


list(APPEND CMAKE_PREFIX_PATH "${EXTERN_PATH}")
find_package(Qt5Widgets)

if(${Qt5Widgets_FOUND})
	set(
		src_basic_gui
		src/bubbles_client_main.cpp
		src/bubbles_client_widget.hpp
		src/bubbles_client_widget.cpp
	)

	add_executable(
		bubbles_client
		${uic_basic_gui}
	#	${res}
		${src_basic_gui}
	)
	
	add_dependencies(bubbles_client bubbles_client_certs)
	
	target_link_libraries(
		bubbles_client
		bubbles_client_engine
		solid_frame_mpipc
		solid_frame_aio_openssl
		solid_serialization
		boost_program_options
		boost_system
		Qt5::Widgets
		snappy
		${SYS_BASIC_LIBS}
	)
endif()
