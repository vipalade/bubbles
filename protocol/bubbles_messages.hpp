#ifndef BUBBLES_MESSAGES_HPP
#define BUBBLES_MESSAGES_HPP

#include "solid/frame/mpipc/mpipcmessage.hpp"
#include "solid/frame/mpipc/mpipccontext.hpp"
#include "solid/frame/mpipc/mpipcprotocol_serialization_v1.hpp"

#include <vector>

namespace bubbles{

struct RegisterRequest: solid::frame::mpipc::Message{
	std::string			room_name;
	uint32_t			rgb_color;
	
	RegisterRequest(){}
	
	RegisterRequest(
		std::string && _uroom_name,
		uint32_t _rgb_color
	): room_name(std::move(_uroom_name)), rgb_color(_rgb_color){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(room_name, "room_name").push(rgb_color, "rgb_color");
	}
};

struct RegisterResponse: solid::frame::mpipc::Message{
	uint32_t		error;
	uint32_t		rgb_color;
	std::string		message;
	
	RegisterResponse(){}
	
	RegisterResponse(
		const RegisterRequest &_rrec, uint32_t _err, const std::string &_msg
	):solid::frame::mpipc::Message(_rrec), error(_err), rgb_color(0), message(_msg){}
	
	RegisterResponse(
		const RegisterRequest &_rrec, uint32_t _rgb_color
	): error(0), rgb_color(_rgb_color){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(error, "error").push(rgb_color, "rgb_color").push(message, "message");
	}
};

struct ConnectionId{
	uint32_t	server_idx;
	uint32_t	server_unq;
	
	uint64_t	connection_idx;
	uint32_t	connection_unq;
	
	ConnectionId():server_idx(solid::InvalidIndex()), server_unq(solid::InvalidIndex()), connection_idx(solid::InvalidIndex()), connection_unq(solid::InvalidIndex()){}
	
	bool isValid()const{
		return server_idx != solid::InvalidIndex();
	}
	
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

//Push notification - no guarantee and no feedback for delivery
struct EventsNotification: solid::frame::mpipc::Message{
	using EventVectorT = std::vector<Event>;
	
	ConnectionId	connection_id;
	uint32_t		sender_rgb_color;
	Event			main_event;
	EventVectorT	events;
	std::string		text;
	
	EventsNotification(){}
	
	void clear(){
		events.clear();
		text.clear();
	}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.pushContainerLimit();//back to default
		_s.pushStringLimit();//back to default
		_s.push(text, "text").pushContainer(events, "events").push(main_event, "main_event").push(sender_rgb_color, "sender_rgb_color").push(connection_id, "connection_id");
		_s.pushContainerLimit(1024);
		_s.pushStringLimit(1024);
	}
};

//Push notification with feedback for delivery
struct EventsNotificationRequest: EventsNotification{
};

struct EventsNotificationResponse: solid::frame::mpipc::Message{
	uint32_t		error;
	uint32_t		success_count;
	uint32_t		fail_count;
	std::string		message;
	
	EventsNotificationResponse():error(0){}
	
	EventsNotificationResponse(
		const EventsNotificationRequest &_req,
		uint32_t _error,
		uint32_t _success_count,
		uint32_t _fail_count,
		const std::string &_rmsg = ""
	):	solid::frame::mpipc::Message(_req), error(_error),
		success_count(_success_count), fail_count(_fail_count), message(_rmsg){}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.push(message, "message").push(fail_count, "fail_count").push(success_count, "success_count").push(error, "error");
	}
};

using ProtoSpecT = solid::frame::mpipc::serialization_v1::ProtoSpec<
	0, RegisterRequest, RegisterResponse, EventsNotification,
	EventsNotificationRequest, EventsNotificationResponse
>;

}//namespace bubbles

#endif
