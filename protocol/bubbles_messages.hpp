#ifndef BUBBLES_MESSAGES_HPP
#define BUBBLES_MESSAGES_HPP

#include "solid/frame/mpipc/mpipcmessage.hpp"
#include "solid/frame/mpipc/mpipccontext.hpp"
#include "solid/frame/mpipc/mpipcprotocol_serialization_v1.hpp"

#include <vector>
#include <deque>

namespace bubbles{

struct RegisterRequest: solid::frame::mpipc::Message{
	std::string			room_name;
	uint32_t			rgb_color;
	
	RegisterRequest(){}
	
	RegisterRequest(
		const std::string &_rroom_name,
		uint32_t _rgb_color
	): room_name(_rroom_name), rgb_color(_rgb_color){}
	
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
	
	bool success()const{
		return error == 0;
	}
	
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
	
	void clear(){
		server_idx = solid::InvalidIndex();
		connection_idx = solid::InvalidIndex();
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
		Sentinel
	};
	
	Event(uint16_t _type = Unknown): type(_type), flags(0), x(0), y(0), data(0){}
	
	void clear(){
		type = Unknown;
		flags = 0;
		x = 0;
		y = 0;
		data = 0;
	}
	
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

// struct InitEventStub{
// 	
// 	Event 			event;
// 	ConnectionId	connection_id;
// 	uint32_t		sender_rgb_color;
// 	std::string		text;
// 	
// 	template <class S>
// 	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
// 		_s.push(text, "text").push(event, "event").push(sender_rgb_color, "sender_rgb_color").push(connection_id, "connection_id");
// 	}
// };
// 
// struct InitNotification: solid::frame::mpipc::Message{
// 	using EventDequeT = std::deque<InitEventStub>;
// 	
// 	static size_t containerLimit(){
// 		return 1024;
// 	}
// 	
// 	EventDequeT		events;
// 	
// 	template <class S>
// 	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
// 		_s.pushContainerLimit();//back to default
// 		_s.pushStringLimit();//back to default
// 		
// 		_s.pushContainer(events, "events");
// 		
// 		_s.pushContainerLimit(containerLimit());
// 		_s.pushStringLimit(1024);
// 	}
// };

struct EventStub{
	using EventVectorT = std::vector<Event>;
	
	Event 			event;
	ConnectionId	connection_id;
	uint32_t		sender_rgb_color;
	std::string		text;
	EventVectorT	events;
	
	bool empty()const{
		return event.type == Event::Unknown;
	}
	
	EventStub(uint16_t _event_type = Event::Unknown): event(_event_type){}
	
	void clear(){
		event.clear();
		connection_id.clear();
		text.clear();
		events.clear();
	}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.pushContainer(events, "events");
		_s.push(text, "text").push(event, "event").push(sender_rgb_color, "sender_rgb_color").push(connection_id, "connection_id");
	}
};

//Push notification - no guarantee and no feedback for delivery
struct EventsNotification: solid::frame::mpipc::Message{
	
	using EventStubDequeT	= std::deque<EventStub>;
	
	EventStub			event_stub;
	EventStubDequeT		event_stubs;
	bool				is_init;//not serialized - used on server
	
	static size_t containerLimit(){
		return 1024;
	}
	
	EventsNotification():is_init(false){}
	
	void clear(){
		event_stubs.clear();
		event_stub.clear();
	}
	
	template <class S>
	void serialize(S &_s, solid::frame::mpipc::ConnectionContext &_rctx){
		_s.pushContainerLimit();//back to default
		_s.pushStringLimit();//back to default
		
		_s.push(event_stub, "event_stub").pushContainer(event_stubs, "event_stubs");
		
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
	0, RegisterRequest, RegisterResponse, /*InitNotification,*/ EventsNotification,
	EventsNotificationRequest, EventsNotificationResponse
>;

}//namespace bubbles

#endif
