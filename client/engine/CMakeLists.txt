
add_library (bubbles_client_engine
    bubbles_client_engine.hpp
    src/bubbles_client_engine.cpp
)

target_link_libraries(bubbles_client_engine solid_frame_mprpc)




