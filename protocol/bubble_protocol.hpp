#ifndef BUBBLES_PROTOCOL_HPP
#define BUBBLES_PROTOCOL_HPP

#include "solid/frame/mpipc/mpipcmessage.hpp"
#include "solid/frame/mpipc/mpipccontext.hpp"
#include "solid/frame/mpipc/mpipcprotocol_serialization_v1.hpp"

namespace bubbles{

struct RegisterRequest: solid::frame::mpipc::Message{
	std::string			room_name;
	uint32_t			rgb_colour;
	
	RegisterRequest(){}
	
	RegisterRequest(
		std::string && _uroom_name,
		uint32_t _rgb_colour
	): room_name(std::move(_uroom_name)), rgb_colour(_rgb_colour){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(room_name, "room_name").push(rgb_colour, "rgb_colour");
	}
};

struct RegisterResponse: solid::frame::mpipc::Message{
	uint32_t		error;
	uint32_t		rgb_colour;
	std::string		error_message;
	
	RegisterResponse(){}
	
	RegisterResponse(
		uint32_t _error, uint32_t _rgb_colour,
		std::string && _uerror_message
	): error(_error), rgb_colour(_rgb_colour), error_message(std::move(_uerror_message)){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(error, "error").push(rgb_colour, "rgb_colour").push(error_message, "error_message");
	}
};

using ProtoSpecT = solid::frame::mpipc::serialization_v1::ProtoSpec<0, RegisterRequest, RegisterResponse, >;

}//namespace bubbles

#endif
