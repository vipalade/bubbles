# the bubble service

add_executable (bubbles_server src/bubbles_server.cpp src/bubbles_server_engine.cpp ../../protocol/bubbles_messages.hpp)

target_link_libraries (bubbles_server
	solid_frame_mpipc
	solid_frame_aio
	solid_frame
	solid_serialization
	solid_utility
	solid_system
	${SYS_BASIC_LIBS}
	${SYS_DYNAMIC_LOAD_LIB}
)

