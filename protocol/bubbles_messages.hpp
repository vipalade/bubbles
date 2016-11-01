#ifndef BUBBLES_MESSAGES_HPP
#define BUBBLES_MESSAGES_HPP

#include "solid/frame/mpipc/mpipcmessage.hpp"
#include "solid/frame/mpipc/mpipccontext.hpp"
#include "solid/frame/mpipc/mpipcprotocol_serialization_v1.hpp"

#include <vector>

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
	std::string		message;
	
	RegisterResponse(){}
	
	RegisterResponse(
		const RegisterRequest &_rrec, uint32_t _err, const std::string &_msg
	):solid::frame::mpipc::Message(_rrec), error(_err), rgb_colour(0), message(_msg){}
	
	RegisterResponse(
		const RegisterRequest &_rrec, uint32_t _rgb_colour
	): error(0), rgb_colour(_rgb_colour){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(error, "error").push(rgb_colour, "rgb_colour").push(message, "message");
	}
};

struct SenderId{
	uint32_t	server_idx;
	uint32_t	server_unq;
	
	uint64_t	connection_idx;
	uint32_t	connection_unq;
	
	SenderId():server_idx(-1), server_unq(-1), connection_idx(-1), connection_unq(-1){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(server_idx, "server_idx").push(server_unq, "server_unq").push(connection_idx, "connection_idx").push(connection_unq, "connection_unq");
	}
};

struct Event{
	enum Types{
		Unknown = 0,
		PointerMove,
	};
	
	Event(): type(Unknown), flags(0), x(0), y(0), data(0){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(diff_time_msec, "diff_time_msec").push(data, "data").push(y, "y").push(x, "x").push(flags, "flags").push(type, "type");
	}
	
	uint16_t	type;
	uint16_t	flags;
	int32_t		x;
	int32_t		y;
	uint64_t	data;
	uint32_t	diff_time_msec;
};

struct EventsNotification: solid::frame::mpipc::Message{
	using EventVectorT = std::vector<Event>;
	
	SenderId		sender_id;
	uint32_t		sender_rgb_colour;
	Event			main_event;
	EventVectorT	events;
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.pushContainer(events, "events").push(main_event, "main_event").push(sender_rgb_colour, "sender_rgb_colour").push(sender_id, "sender_id");
	}
};


using ProtoSpecT = solid::frame::mpipc::serialization_v1::ProtoSpec<0, RegisterRequest, RegisterResponse, EventsNotification>;

}//namespace bubbles

#endif
