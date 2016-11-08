#include <memory>
#include <deque>
#include <stack>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <unordered_set>

#include "bubbles_server_engine.hpp"
#include "solid/frame/mpipc/mpipccontext.hpp"
#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/system/debug.hpp"
#include "solid/system/cassert.hpp"
#include "solid/utility/any.hpp"

using namespace solid;
using namespace std;

namespace bubbles{
namespace server{

enum Error{
	ErrorAlreadyRegistered = 1,
	ErrorNoColour,
};

const char * error_message(const uint32_t err_id){
	switch(err_id){
		case 0: return "No error";
		case ErrorAlreadyRegistered: return "Already registered - closing connection";
		case ErrorNoColour: return "No color available - closing connection";
		default:
			return "Unknown error";
	}
}

struct ConnectionData{
	ConnectionData(): room_index(solid::InvalidIndex()), room_entry_index(solid::InvalidIndex()){}
	
	bool registered()const{
		return room_index != solid::InvalidIndex();
	}
	
	void clear(){
		room_index = -1;
		room_entry_index = -1;
	}
	
	size_t		room_index;
	size_t		room_entry_index;
};

using ConnectionId = solid::frame::mpipc::RecipientId;

struct ConnectionStub{
	
	size_t					pending_count;
	size_t					dropped_message_count;
	size_t					crt_fetch_pos;
	ConnectionId			id;
	Event					last_event;
	std::string				last_text;
	uint32_t				rgb_color;
	
	void clear(){
		pending_count = 0;
		id.clear();
		last_text.clear();
		last_event.clear();
	}
	
	bool canAcceptEventsNotification(const EngineConfiguration &_rconfig, const EventsNotification& /*_rmsg*/, const size_t _sender_index)const{
		return id.isValidPool() and pending_count < _rconfig.connection_max_pending_count and crt_fetch_pos > _sender_index;
	}
	
	bool canAcceptEventsNotification(const EngineConfiguration &_rconfig)const{
		return id.isValidPool() and pending_count < _rconfig.connection_max_pending_count;
	}
	
	void saveLastEvent(const EventsNotification& _rmsg){
		last_event = _rmsg.events.size() ? _rmsg.events.back() : _rmsg.main_event;
		if(_rmsg.text.size()){
			last_text = _rmsg.text;
		}
	}
	
	bool hasLastEvent()const{
		return last_event.type == Event::Unknown;
	}
	
	ConnectionStub():pending_count(0), dropped_message_count(0), crt_fetch_pos(solid::InvalidIndex{}){}
};

using ConnectionVectorT = deque<ConnectionStub>;
using FreeStackT = stack<size_t>;
using ColourSetT = unordered_set<uint32_t>;


struct RoomStub{
	RoomStub():crt_rgb_color(0), crt_rgb_color_step(255){}
	
	string				name;
	ConnectionVectorT	connections;
	FreeStackT			free_stack;
	ColourSetT			used_colors;
	uint32_t			crt_rgb_color;
	uint32_t			crt_rgb_color_step;
	
	bool empty()const{
		return connections.size() == free_stack.size();
	}
	
	void clear(){
		name.clear();
		while(free_stack.size()) free_stack.pop();
		
		connections.clear();
	}
};

using RoomVectorT = deque<RoomStub>;


struct StringPtrHash{
	size_t operator()(const string * _ps)const{
		return std::hash<string>{}(*_ps);
	}
};

using NameMapT = unordered_map<const string*, size_t, StringPtrHash>;

struct Engine::Data{
	Data(const EngineConfiguration &_config):config(_config), max_dropped_message_count(0){}
	
	RoomVectorT				rooms;
	FreeStackT				free_stack;
	NameMapT				room_map;
	EngineConfiguration		config;
	size_t					max_dropped_message_count;
};

Engine::Engine(const EngineConfiguration &_config):d(*(new Data(_config))){}

Engine::~Engine(){
	delete &d;
}

void Engine::plotStatistics(std::ostream &_ros){
	_ros<<"Max per connection dropped messages: "<<d.max_dropped_message_count<<endl;
}

void Engine::onConnectionStart(solid::frame::mpipc::ConnectionContext &_rctx){
	idbg(_rctx.recipientId());
	
	_rctx.any() = solid::make_any<0, ConnectionData>();
}

void Engine::onConnectionStop(solid::frame::mpipc::ConnectionContext &_rctx){
	idbg(_rctx.recipientId());
	
	ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();
	unregisterConnection(_rctx, rcon_data);
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
	std::shared_ptr<RegisterRequest> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
	SOLID_ASSERT(_rrecv_msg_ptr);
	SOLID_ASSERT(not _rsent_msg_ptr);
	ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();
	
	uint32_t	error_id = 0;
	
	if(not rcon_data.registered()){
		uint32_t	rgb_color;
		error_id = registerConnection(_rctx, rcon_data, *_rrecv_msg_ptr, rgb_color);
		
		if(error_id == 0){
			_rctx.service().sendMessage(
				_rctx.recipientId(),
				std::make_shared<RegisterResponse>(*_rrecv_msg_ptr, rgb_color), 0|frame::mpipc::MessageFlags::Synchronous
			);
		}
	}else{
		error_id = ErrorAlreadyRegistered;
	}
	
	_rctx.service().sendMessage(
		_rctx.recipientId(),
		std::make_shared<RegisterResponse>(*_rrecv_msg_ptr, error_id, error_message(error_id)), 0|frame::mpipc::MessageFlags::Synchronous
	);
	_rctx.service().delayCloseConnectionPool(_rctx.recipientId(), [](frame::mpipc::ConnectionContext &/*_rctx*/){});
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<RegisterResponse> &_rsent_msg_ptr,
	std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
	SOLID_ASSERT(not _rrecv_msg_ptr);
	SOLID_ASSERT(_rsent_msg_ptr);
	if(_rrecv_msg_ptr){
		_rctx.service().closeConnection(_rctx.recipientId());
	}
}

// void Engine::onMessage(
// 	solid::frame::mpipc::ConnectionContext &_rctx,
// 	std::shared_ptr<InitNotification> &_rsent_msg_ptr,
// 	std::shared_ptr<InitNotification> &_rrecv_msg_ptr,
// 	solid::ErrorConditionT const &_rerror
// ){
// 	if(_rsent_msg_ptr){
// 		idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
// 		
// 		ConnectionData 		&rcon_data = *_rctx.any().cast<ConnectionData>();
// 			
// 		RoomStub			&room = d.rooms[rcon_data.room_index];
// 		ConnectionStub		&rcon = room.connections[rcon_data.room_entry_index];
// 		
// 		rcon.pending_count -= (1 + _rsent_msg_ptr->events.size());
// 		
// 		
// 		if(rcon.crt_fetch_pos < room.connections.size()){
// 			fetchLastEvents(_rctx, rcon_data, std::move(_rsent_msg_ptr));
// 		}
// 	}
// }

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
	std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
	
	if(_rrecv_msg_ptr){
		//received message
		
		ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();
		
		if(rcon_data.registered()){
			RoomStub	&room = d.rooms[rcon_data.room_index];
			
			
			ConnectionStub		&rcon_sender = room.connections[rcon_data.room_entry_index];
			
			rcon_sender.saveLastEvent(*_rrecv_msg_ptr);
			//TODO: set:
			//_rrecv_msg_ptr->connection_id
			
			_rrecv_msg_ptr->event_stub.sender_rgb_color = rcon_sender.rgb_color;
			
			for(size_t i = 0; i < room.connections.size(); ++i){
				ConnectionStub &rcon = room.connections[i];
				if(i != rcon_data.room_entry_index and rcon.canAcceptEventsNotification(d.config, *_rrecv_msg_ptr, rcon_data.room_entry_index)){
					rcon.pending_count += (1 + _rrecv_msg_ptr->events.size());
					_rctx.service().sendMessage(rcon.id, _rrecv_msg_ptr, 0|frame::mpipc::MessageFlags::Synchronous);
				}
			}
		}else{
			_rctx.service().closeConnection(_rctx.recipientId());
		}
	}else if(_rsent_msg_ptr and _rsent_msg_ptr->is_init){
		idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
		
		ConnectionData 		&rcon_data = *_rctx.any().cast<ConnectionData>();
			
		RoomStub			&room = d.rooms[rcon_data.room_index];
		ConnectionStub		&rcon = room.connections[rcon_data.room_entry_index];
		
		rcon.pending_count -= (1 + _rsent_msg_ptr->event_stubs.size());
		
		
		if(rcon.crt_fetch_pos < room.connections.size()){
			fetchLastEvents(_rctx, rcon_data, std::move(_rsent_msg_ptr));
		}
	}
}

void Engine::fetchLastEvents(
	solid::frame::mpipc::ConnectionContext &_rctx, ConnectionData &_rcon_data,
	std::shared_ptr<EventsNotification> &&_msg_ptr
){
	RoomStub			&room = d.rooms[_rcon_data.room_index];
	ConnectionStub		&rcrtcon = room.connections[_rcon_data.room_entry_index];
	
	_msg_ptr->is_init = true;
	_msg_ptr->event_stubs.clear();
	
	//continue fetch last events 
	while(rcrtcon.crt_fetch_pos < room.connections.size() and _msg_ptr->event_stubs.size() < EventsNotification::containerLimit()){
		
		ConnectionStub &rcon = room.connections[rcon.crt_fetch_pos];
		
		if(rcon.crt_fetch_pos != _rcon_data.room_entry_index and rcon.hasLastEvent()){
			
			if(not _msg_ptr->event_stub.empty()){
				_msg_ptr->event_stubs.push_back(EventStub{});
			}
			EventStub &back_event = _msg_ptr->event_stub.empty() ?  _msg_ptr->event_stub : _msg_ptr->event_stubs.back();
			
			back_event.event = rcon.last_event;
			back_event.text = rcon.last_text;
			back_event.sender_rgb_color = rcon.rgb_color;
			//back_event.connection_id = rcon.id;//TODO:
		}
		++rcon.crt_fetch_pos;
	}
	if(rcrtcon.crt_fetch_pos == room.connections.size()){
		
		if(_msg_ptr->event_stubs.size() < EventsNotification::containerLimit()){
			//add one last sentinel event
			if(not _msg_ptr->event_stub.empty()){
				_msg_ptr->event_stubs.push_back(EventStub{Event::Sentinel});
			}else{
				_msg_ptr->event_stub.event.type = Event::Sentinel;
			}
			rcrtcon.crt_fetch_pos = solid::InvalidIndex{};
		}
	}
	if(_msg_ptr->event_stubs.size() or not _msg_ptr->event_stub.empty()){
		_rctx.service().sendMessage(rcrtcon.id, _msg_ptr, 0|frame::mpipc::MessageFlags::Synchronous);
	}
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
	std::shared_ptr<EventsNotificationRequest> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
	//TODO:
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<EventsNotificationResponse> &_rsent_msg_ptr,
	std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
	//TODO:
}

uint32_t Engine::registerConnection(
	solid::frame::mpipc::ConnectionContext &_rctx,
	ConnectionData &_rcon_data,
	const RegisterRequest &_rreq,
	uint32_t &_rrgb_color
){
	std::string room_name;
	
	uint32_t	rgb_color = 0;
	
	room_name.resize(_rreq.room_name.size());
	std::transform(_rreq.room_name.begin(), _rreq.room_name.end(), room_name.begin(), ::tolower);
	
	{
		auto it = d.room_map.find(&room_name);
		if(it != d.room_map.end()){
			_rcon_data.room_index = it->second;
		}else{
			//allocate new room
			if(d.free_stack.size()){
				_rcon_data.room_index = d.free_stack.top();
				d.free_stack.pop();
			}else{
				_rcon_data.room_index = d.rooms.size();
				d.rooms.push_back(RoomStub{});
				d.rooms.back().name = std::move(room_name);
			}
			d.room_map[&d.rooms[_rcon_data.room_index].name] = _rcon_data.room_index;
		}
	}
	
	RoomStub	&room = d.rooms[_rcon_data.room_index];
	
	if(_rreq.rgb_color){
		//client requested an explicit color - see if it can be used
		
		auto it = room.used_colors.find(_rreq.rgb_color);
		
		if(it != room.used_colors.end()){
			rgb_color = _rreq.rgb_color;
		}
	}
	
	if(rgb_color == 0){
		rgb_color = createNewColour(_rcon_data.room_index);
	}
	
	if(rgb_color == 0){
		_rcon_data.clear();
		return ErrorNoColour;
	}
	
	//register connection:
	if(room.free_stack.size()){
		_rcon_data.room_entry_index = room.free_stack.top();
		room.free_stack.pop();
	}else{
		_rcon_data.room_entry_index = room.connections.size();
		room.connections.push_back(ConnectionStub{});
	}
	
	
	ConnectionStub &rcon = room.connections[_rcon_data.room_entry_index];
	
	rcon.id = _rctx.recipientId();
	
	rcon.crt_fetch_pos = 0;
	rcon.rgb_color = rgb_color;
	_rrgb_color = rgb_color;
	
	fetchLastEvents(_rctx, _rcon_data, std::make_shared<InitNotification>());
	
	return 0;
}

void Engine::unregisterConnection(solid::frame::mpipc::ConnectionContext &_rctx, ConnectionData &_rcon_data){
	RoomStub	&room = d.rooms[_rcon_data.room_index];
	{
		const size_t		dropped_msg_count = room.connections[_rcon_data.room_entry_index].dropped_message_count;
		
		if(d.max_dropped_message_count < dropped_msg_count){
			d.max_dropped_message_count = dropped_msg_count;
		}
	}
	
	{
		ConnectionStub		&rcon_sender = room.connections[_rcon_data.room_entry_index];
				
		auto close_msg_ptr = std::make_shared<EventsNotification>();
		close_msg_ptr->sender_rgb_color = rcon_sender.rgb_color;
		//TODO: set:
		//_rrecv_msg_ptr->connection_id
		
		
		for(size_t i = 0; i < room.connections.size(); ++i){
			ConnectionStub &rcon = room.connections[i];
			if(i != _rcon_data.room_entry_index){
				_rctx.service().sendMessage(rcon.id, close_msg_ptr, 0|frame::mpipc::MessageFlags::Synchronous);
			}
		}
	}
	
	room.used_colors.erase(room.connections[_rcon_data.room_entry_index].rgb_color);
	
	room.connections[_rcon_data.room_entry_index].clear();
	room.free_stack.push(_rcon_data.room_entry_index);
	
	
	if(room.empty()){
		d.room_map.erase(&room.name);
		
		room.clear();
		d.free_stack.push(_rcon_data.room_index);
	}
	
	_rcon_data.room_index = -1;
	_rcon_data.room_entry_index = -1;
}


uint32_t Engine::createNewColour(const size_t _room_index){
	RoomStub		&room = d.rooms[_room_index];
	const uint32_t	max_rgb_color = 0xffffff00;
	
	room.crt_rgb_color += room.crt_rgb_color_step;
	
	if(room.crt_rgb_color > max_rgb_color){
		if(room.crt_rgb_color_step > 27){
			room.crt_rgb_color_step -= 17;
			room.crt_rgb_color = room.crt_rgb_color_step;
		}else{
			return 0;
		}
	}
	return room.crt_rgb_color;
}

}//namespace server
}//namespace bubbles
