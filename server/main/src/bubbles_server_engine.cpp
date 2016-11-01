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
		case ErrorNoColour: return "No colour available - closing connection";
		default:
			return "Unknown error";
	}
}

struct ConnectionData{
	ConnectionData(): room_index(-1), room_entry_index(-1), rgb_colour(0){}
	
	bool registered()const{
		return rgb_colour != 0;
	}
	
	void clear(){
		room_index = -1;
		room_entry_index = -1;
		rgb_colour = 0;
	}
	
	size_t		room_index;
	size_t		room_entry_index;
	uint32_t	rgb_colour;
	
};

using ConnectionId = solid::frame::mpipc::RecipientId;
using ConnectionVectorT = deque<ConnectionId>;
using FreeStackT = stack<size_t>;
using ColourSetT = unordered_set<uint32_t>;

struct RoomStub{
	RoomStub():crt_rgb_colour(0), crt_rgb_colour_step(255);
	
	string				name;
	ConnectionVectorT	connections;
	FreeStackT			free_stack;
	ColourSetT			used_colours;
	uint32_t			crt_rgb_colour;
	uint32_t			crt_rgb_colour_step;
	
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
	RoomVectorT		rooms;
	FreeStackT		free_stack;
	NameMapT		room_map;
};

Engine::Engine():d(*(new Data)){}


void Engine::onConnectionStart(solid::frame::mpipc::ConnectionContext &_rctx){
	idbg(_rctx.recipientId());
	
	_rctx.any() = solid::make_any<0, ConnectionData>();
}

void Engine::onConnectionStop(solid::frame::mpipc::ConnectionContext &_rctx){
	idbg(_rctx.recipientId());
	
	ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();
	unregisterConnection(rcon_data);
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
	std::shared_ptr<RegisterRequest> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId());
	SOLID_ASSERT(_rrecv_msg_ptr);
	SOLID_ASSERT(not _rsent_msg_ptr);
	ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();
	
	uint32_t	error_id = 0;
	
	if(not rcon_data.registered()){
		
		error_id = registerConnection(_rctx, rcon_data, *_rrecv_msg_ptr);
		
		if(error_id == 0){
			_rctx.service().sendMessage(
				_rctx.recipientId(),
				std::make_shared<RegisterResponse>(*_rrecv_msg_ptr, rcon_data.rgb_colour)
			);
		}
	}else{
		error_id = ErrorAlreadyRegistered;
	}
	
	_rctx.service().sendMessage(
		_rctx.recipientId(),
		std::make_shared<RegisterResponse>(*_rrecv_msg_ptr, error_id, error_message(error_id))
	);
	_rctx.service().delayCloseConnectionPool(_rctx.recipientId(), [](frame::mpipc::ConnectionContext &/*_rctx*/){});
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<RegisterResponse> &_rsent_msg_ptr,
	std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId());
	SOLID_ASSERT(not _rrecv_msg_ptr);
	SOLID_ASSERT(_rsent_msg_ptr);
	if(_rrecv_msg_ptr){
		_rctx.service().closeConnection(_rctx.recipientId());
	}
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
	std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId());
	if(_rrecv_msg_ptr){
		//only care about received message
		
		ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();
		
		if(rcon_data.registered()){
			const RoomStub	&room = d.rooms[rcon_data.room_index];
			
			_rrecv_msg_ptr->sender_rgb_colour = rcon_data.rgb_colour;
			
			for(size_t i = 0; i < room.connections.size(); ++i){
				const solid::frame::mpipc::RecipientId &recipient_id = room.connections[i];
				if(i != rcon_data.room_entry_index and recipient_id.isValidPool()){
					_rctx.service().sendMessage(recipient_id, _rrecv_msg_ptr);
				}
			}
		}else{
			_rctx.service().closeConnection(_rctx.recipientId());
		}
	}
}


uint32_t Engine::registerConnection(
	solid::frame::mpipc::ConnectionContext &_rctx,
	ConnectionData &_rcon_data,
	const RegisterRequest &_rreq
){
	std::string room_name;
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
	
	if(_rreq.rgb_colour){
		//client requested an explicit colour - see if it can be used
		
		auto it = room.used_colours.find(_rreq.rgb_colour);
		
		if(it != room.used_colours.end()){
			_rcon_data.rgb_colour = _rreq.rgb_colour;
		}
	}
	
	if(_rcon_data.rgb_colour == 0){
		_rcon_data.rgb_colour = createNewColour(_rcon_data.room_index);
	}
	
	if(_rcon_data.rgb_colour == 0){
		_rcon_data.clear();
		return ErrorNoColour;
	}
	
	//register connection:
	if(room.free_stack.size()){
		_rcon_data.room_entry_index = room.free_stack.top();
		room.free_stack.pop();
	}else{
		_rcon_data.room_entry_index = room.connections.size();
		room.connections.push_back(ConnectionId{});
	}
	room.connections[_rcon_data.room_entry_index] = _rctx.recipientId();
	
	return 0;
}

void Engine::unregisterConnection(ConnectionData &_rcon_data){
	RoomStub	&room = d.rooms[_rcon_data.room_index];
	
	room.connections[_rcon_data.room_entry_index].clear();
	room.free_stack.push(_rcon_data.room_entry_index);
	
	room.used_colours.erase(_rcon_data.rgb_colour);
	
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
	const uint32_t	max_rgb_colour = 0xffffff00;
	
	room.crt_rgb_colour += room.crt_rgb_colour_step;
	
	if(room.crt_rgb_colour > max_rgb_colour){
		if(room.crt_rgb_colour_step > 27){
			room.crt_rgb_colour_step -= 17;
			room.crt_rgb_colour = room.crt_rgb_colour_step;
		}else{
			return 0;
		}
	}
	return room.crt_rgb_colour;
}

}//namespace server
}//namespace bubbles
