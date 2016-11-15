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

	target_link_libraries(
		bubbles_client
		bubbles_client_engine
		solid_frame_mpipc
		solid_frame_aio
		solid_frame
		solid_serialization
		solid_utility
		solid_system
		boost_program_options
		boost_system
		Qt5::Widgets
		${SYS_BASIC_LIBS}
	)
endif()
