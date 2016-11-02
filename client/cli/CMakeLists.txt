# the bubble client_cli

add_executable (bubbles_client_cli src/bubbles_client_cli.cpp ../../protocol/bubbles_messages.hpp)

target_link_libraries (bubbles_client_cli
	bubbles_client_engine
	solid_frame_mpipc
	solid_frame_aio
	solid_frame
	solid_serialization
	solid_utility
	solid_system
	boost_program_options
	boost_system
	${SYS_BASIC_LIBS}
	${SYS_DYNAMIC_LOAD_LIB}
)


