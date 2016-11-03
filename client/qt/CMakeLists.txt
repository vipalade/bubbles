INCLUDE("../../cmake/find_qt.cmake")

# qt5_wrap_ui(
# 	uic_basic_gui
# 	main_window.ui
# )

# qt5_add_resources(
# 	res
# 	icons.qrc
# )

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

qt5_use_modules (bubbles_client Core Widgets DBus)

set_target_properties(
	bubbles_client
	PROPERTIES
	AUTOMOC TRUE
)

target_include_directories(
	bubbles_client
	PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
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
	${QT_LIBRARIES}
	${SYS_BASIC_LIBS}
)
