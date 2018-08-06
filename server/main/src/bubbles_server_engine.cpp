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
#include "solid/system/log.hpp"
#include "solid/system/cassert.hpp"
#include "solid/utility/any.hpp"

using namespace solid;
using namespace std;

namespace bubbles{
namespace server{

enum Error{
    ErrorAlreadyRegistered = 1,
    ErrorNoColor,
};

const char * error_message(const uint32_t err_id){
    switch(err_id){
        case 0: return "No error";
        case ErrorAlreadyRegistered: return "Already registered - closing connection";
        case ErrorNoColor: return "No color available - closing connection";
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

    size_t      room_index;
    size_t      room_entry_index;
};

using ConnectionId = solid::frame::mpipc::RecipientId;

struct ConnectionStub{

    size_t                  pending_count;
    size_t                  dropped_message_count;
    size_t                  crt_fetch_pos;
    ConnectionId            id;
    Event                   last_event;
    std::string             last_text;
    uint32_t                rgb_color;

    void clear(){
        pending_count = 0;
        crt_fetch_pos = solid::InvalidIndex();
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

        if(_rmsg.event_stubs.size()){
            if(_rmsg.event_stubs.back().events.size()){
                last_event = _rmsg.event_stubs.back().events.back();
            }else{
                last_event = _rmsg.event_stubs.back().event;
            }
            if(_rmsg.event_stubs.back().text.size()){
                last_text = _rmsg.event_stubs.back().text;
            }
        }else{
            if(_rmsg.event_stub.events.size()){
                last_event = _rmsg.event_stub.events.back();
            }else{
                last_event = _rmsg.event_stub.event;
            }
            last_text = _rmsg.event_stub.text;
        }
    }

    bool hasLastEvent()const{
        return last_event.type != Event::Unknown;
    }

    ConnectionStub():pending_count(0), dropped_message_count(0), crt_fetch_pos(solid::InvalidIndex{}){}
};

using ConnectionVectorT = deque<ConnectionStub>;
using FreeStackT = stack<size_t>;
using ColorSetT = unordered_set<uint32_t>;


struct RoomStub{
    RoomStub():crt_r_color(0), crt_g_color(0), crt_b_color(0), crt_rgb_color_step(255), crt_color_pos(0){}

    string              name;
    ConnectionVectorT   connections;
    FreeStackT          free_stack;
    ColorSetT          used_colors;

    uint32_t            crt_r_color;
    uint32_t            crt_g_color;
    uint32_t            crt_b_color;

    uint32_t            crt_rgb_color_step;
    uint16_t            crt_color_pos;

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

struct StringPtrEqual{
    bool operator()(const string * _ps1, const string *_ps2)const{
        return *_ps1 == *_ps2;
    }
};

using NameMapT = unordered_map<const string*, size_t, StringPtrHash, StringPtrEqual>;

struct Engine::Data{
    Data(const EngineConfiguration &_config):config(_config), max_dropped_message_count(0){}

    RoomVectorT             rooms;
    FreeStackT              free_stack;
    NameMapT                room_map;
    EngineConfiguration     config;
    size_t                  max_dropped_message_count;
};

Engine::Engine(const EngineConfiguration &_config):d(*(new Data(_config))){}

Engine::~Engine(){
    delete &d;
}

void Engine::plotStatistics(std::ostream &_ros){
    _ros<<"Max per connection dropped messages: "<<d.max_dropped_message_count<<endl;
}

void Engine::onConnectionStart(solid::frame::mpipc::ConnectionContext &_rctx){
    solid_log(generic_logger, Info, _rctx.recipientId());

    _rctx.any() = solid::make_any<0, ConnectionData>();
}

void Engine::onConnectionStop(solid::frame::mpipc::ConnectionContext &_rctx){
    solid_log(generic_logger, Info, _rctx.recipientId()<<' '<<_rctx.error().message());

    ConnectionData *pcon_data = _rctx.any().cast<ConnectionData>();

    if(pcon_data and pcon_data->registered()){
        unregisterConnection(_rctx, *pcon_data);
    }
}

void Engine::onMessage(
    solid::frame::mpipc::ConnectionContext &_rctx,
    std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
    std::shared_ptr<RegisterRequest> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){
    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());
    solid_assert(_rrecv_msg_ptr);
    solid_assert(not _rsent_msg_ptr);
    ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();

    uint32_t    error_id = 0;
    solid::ErrorConditionT  err;

    if(not rcon_data.registered()){
        uint32_t    rgb_color;
        error_id = registerConnection(_rctx, rcon_data, *_rrecv_msg_ptr, rgb_color);

        if(error_id == 0){
            
            solid_check(!(err = _rctx.service().sendMessage(
                _rctx.recipientId(),
                std::make_shared<RegisterResponse>(*_rrecv_msg_ptr, rgb_color), {frame::mpipc::MessageFlagsE::Synchronous}
            )), "failed send message: "<<err.message());
            return;
        }
    }else{
        error_id = ErrorAlreadyRegistered;
    }

    solid_check(!(err = _rctx.service().sendResponse(
        _rctx.recipientId(),
        std::make_shared<RegisterResponse>(*_rrecv_msg_ptr, error_id, error_message(error_id)), {frame::mpipc::MessageFlagsE::Synchronous}
    )), "failed send message: "<<err.message());
    _rctx.service().delayCloseConnectionPool(_rctx.recipientId(), [](frame::mpipc::ConnectionContext &/*_rctx*/){});
}

void Engine::onMessage(
    solid::frame::mpipc::ConnectionContext &_rctx,
    std::shared_ptr<RegisterResponse> &_rsent_msg_ptr,
    std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){
    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());
    solid_assert(not _rrecv_msg_ptr);
    solid_assert(_rsent_msg_ptr);
    if(_rrecv_msg_ptr){
        solid_log(generic_logger, Info, _rctx.recipientId()<<"closing connection");
        _rctx.service().closeConnection(_rctx.recipientId());
    }
}

void Engine::onMessage(
    solid::frame::mpipc::ConnectionContext &_rctx,
    std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
    std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){
    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());

    if(_rrecv_msg_ptr){
        //received message

        ConnectionData &rcon_data = *_rctx.any().cast<ConnectionData>();

        if(rcon_data.registered()){
            RoomStub    &room = d.rooms[rcon_data.room_index];


            ConnectionStub      &rcon_sender = room.connections[rcon_data.room_entry_index];

            rcon_sender.saveLastEvent(*_rrecv_msg_ptr);
            //TODO: set:
            //_rrecv_msg_ptr->connection_id
            _rrecv_msg_ptr->clearHeader(); // cumbersome - for now!
            _rrecv_msg_ptr->event_stub.sender_rgb_color = rcon_sender.rgb_color;

            for(size_t i = 0; i < room.connections.size(); ++i){

                ConnectionStub &rcon = room.connections[i];

                if(i != rcon_data.room_entry_index and rcon.canAcceptEventsNotification(d.config, *_rrecv_msg_ptr, rcon_data.room_entry_index)){
                    rcon.pending_count += (1 + _rrecv_msg_ptr->event_stub.events.size());
                    rcon.pending_count += _rrecv_msg_ptr->event_stubs.size();
                    _rrecv_msg_ptr->clearStateFlags();
                    solid_log(generic_logger, Info, i<<" pend_cnt = "<<rcon.pending_count<<" eventsz = "<<_rrecv_msg_ptr->event_stub.events.size());
                    solid::ErrorConditionT err = _rctx.service().sendMessage(rcon.id, _rrecv_msg_ptr, {frame::mpipc::MessageFlagsE::Synchronous});
                    if(err){
                        solid_log(generic_logger, Warning, rcon_data.room_entry_index<<" failed send message: "<<err.message());
                        rcon.pending_count -= (1 + _rrecv_msg_ptr->event_stub.events.size());
                        rcon.pending_count -= _rrecv_msg_ptr->event_stubs.size();
                    }
                }else if(i != rcon_data.room_entry_index){
                    solid_log(generic_logger, Info, _rctx.recipientId()<<" not sent to "<<i<<" pending count = "<<rcon.pending_count);
                }
            }
        }else{
            _rctx.service().closeConnection(_rctx.recipientId());
        }
    }else if(_rsent_msg_ptr){
        solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());

        ConnectionData      &rcon_data = *_rctx.any().cast<ConnectionData>();
        RoomStub            &room = d.rooms[rcon_data.room_index];
        ConnectionStub      &rcon = room.connections[rcon_data.room_entry_index];


        rcon.pending_count -= (1 + _rsent_msg_ptr->event_stub.events.size());
        rcon.pending_count -= _rsent_msg_ptr->event_stubs.size();
        
        solid_log(generic_logger, Info, rcon_data.room_entry_index<<" pend_cnt = "<<rcon.pending_count<<" eventsz = "<<_rsent_msg_ptr->event_stub.events.size());

        if(rcon.crt_fetch_pos < room.connections.size()){
            fetchLastEvents(_rctx, rcon_data, std::make_shared<EventsNotification>());
        }
    }
}

void Engine::fetchLastEvents(
    solid::frame::mpipc::ConnectionContext &_rctx, ConnectionData &_rcon_data,
    std::shared_ptr<EventsNotification> &&_msg_ptr
){
    RoomStub            &room = d.rooms[_rcon_data.room_index];
    ConnectionStub      &rcrtcon = room.connections[_rcon_data.room_entry_index];

    _msg_ptr->is_init = true;
    _msg_ptr->event_stubs.clear();

    //continue fetch last events
    while(rcrtcon.crt_fetch_pos < room.connections.size() and _msg_ptr->event_stubs.size() < EventsNotification::containerLimit()){

        ConnectionStub &rcon = room.connections[rcrtcon.crt_fetch_pos];

        if(rcrtcon.crt_fetch_pos != _rcon_data.room_entry_index and rcon.hasLastEvent()){

            if(not _msg_ptr->event_stub.empty()){
                _msg_ptr->event_stubs.push_back(EventStub{});
            }
            EventStub &back_event = _msg_ptr->event_stub.empty() ?  _msg_ptr->event_stub : _msg_ptr->event_stubs.back();

            back_event.event = rcon.last_event;
            back_event.text = rcon.last_text;
            back_event.sender_rgb_color = rcon.rgb_color;
            //back_event.connection_id = rcon.id;//TODO:
        }
        ++rcrtcon.crt_fetch_pos;
    }
    if(rcrtcon.crt_fetch_pos == room.connections.size()){

//      if(_msg_ptr->event_stubs.size() < EventsNotification::containerLimit()){
//          //add one last sentinel event
//          if(not _msg_ptr->event_stub.empty()){
//              _msg_ptr->event_stubs.push_back(EventStub{Event::Sentinel});
//          }else{
//              _msg_ptr->event_stub.event.type = Event::Sentinel;
//          }
//          rcrtcon.crt_fetch_pos = solid::InvalidIndex{};
//      }
        rcrtcon.crt_fetch_pos = solid::InvalidIndex{};
    }
    if(_msg_ptr->event_stubs.size() or not _msg_ptr->event_stub.empty()){
        rcrtcon.pending_count += _msg_ptr->event_stubs.size();
        rcrtcon.pending_count += (1 + _msg_ptr->event_stub.events.size());
        solid::ErrorConditionT  err =_rctx.service().sendMessage(rcrtcon.id, _msg_ptr, {frame::mpipc::MessageFlagsE::Synchronous});
        if(err){
            solid_log(generic_logger, Warning, _rcon_data.room_entry_index<<" failed send message: "<<err.message());
            rcrtcon.pending_count -= _msg_ptr->event_stubs.size();
            rcrtcon.pending_count -= (1 + _msg_ptr->event_stub.events.size());
        }
    }
}

void Engine::onMessage(
    solid::frame::mpipc::ConnectionContext &_rctx,
    std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
    std::shared_ptr<EventsNotificationRequest> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){
    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());
    //TODO:
}

void Engine::onMessage(
    solid::frame::mpipc::ConnectionContext &_rctx,
    std::shared_ptr<EventsNotificationResponse> &_rsent_msg_ptr,
    std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){
    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());
    //TODO:
}

uint32_t Engine::registerConnection(
    solid::frame::mpipc::ConnectionContext &_rctx,
    ConnectionData &_rcon_data,
    const RegisterRequest &_rreq,
    uint32_t &_rrgb_color
){

    solid_log(generic_logger, Info, _rctx.recipientId()<<" room name "<<_rreq.room_name);

    std::string room_name;

    uint32_t    rgb_color = 0;

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
            }
            d.rooms[_rcon_data.room_index].name = std::move(room_name);
            d.room_map[&d.rooms[_rcon_data.room_index].name] = _rcon_data.room_index;
        }
    }

    RoomStub    &room = d.rooms[_rcon_data.room_index];

    if(_rreq.rgb_color){
        //client requested an explicit color - see if it can be used

        auto it = room.used_colors.find(_rreq.rgb_color);

        if(it == room.used_colors.end()){
            rgb_color = _rreq.rgb_color;
        }
    }

    if(rgb_color == 0){
        rgb_color = createNewColor(_rcon_data.room_index);
    }

    if(rgb_color == 0){
        _rcon_data.clear();
        return ErrorNoColor;
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

    solid_log(generic_logger, Info, "Registered connection on room "<<room.name<<" with id "<<_rcon_data.room_entry_index);

    fetchLastEvents(_rctx, _rcon_data, std::make_shared<EventsNotification>());

    return 0;
}

void Engine::unregisterConnection(solid::frame::mpipc::ConnectionContext &_rctx, ConnectionData &_rcon_data){
    solid_log(generic_logger, Warning, " room: "<<_rcon_data.room_index<<" connection: "<<_rcon_data.room_entry_index);
    RoomStub    &room = d.rooms[_rcon_data.room_index];
    {
        const size_t        dropped_msg_count = room.connections[_rcon_data.room_entry_index].dropped_message_count;

        if(d.max_dropped_message_count < dropped_msg_count){
            d.max_dropped_message_count = dropped_msg_count;
        }
    }

    {
        ConnectionStub      &rcon_sender = room.connections[_rcon_data.room_entry_index];

        auto close_msg_ptr = std::make_shared<EventsNotification>();
        close_msg_ptr->event_stub.sender_rgb_color = rcon_sender.rgb_color;
        //TODO: set:
        //_rrecv_msg_ptr->connection_id


        for(size_t i = 0; i < room.connections.size(); ++i){
            ConnectionStub &rcon = room.connections[i];
            if(i != _rcon_data.room_entry_index){
                rcon.pending_count += 1;
                solid::ErrorConditionT err = _rctx.service().sendMessage(rcon.id, close_msg_ptr, {frame::mpipc::MessageFlagsE::Synchronous});
                if(err){
                    rcon.pending_count -= 1;
                }
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
        solid_log(generic_logger, Warning, " room: "<<_rcon_data.room_index<<" is empty");
    }

    _rcon_data.room_index = -1;
    _rcon_data.room_entry_index = -1;
}


uint32_t Engine::createNewColor(const size_t _room_index){
    RoomStub        &room = d.rooms[_room_index];
    uint32_t color = 0;

    switch(room.crt_color_pos % 6){
        case 0://r
            room.crt_r_color += room.crt_rgb_color_step;
            color = room.crt_r_color; break;
        case 1://g
            room.crt_g_color += room.crt_rgb_color_step;
            color = room.crt_g_color << 8; break;
        case 2://b
            room.crt_b_color += room.crt_rgb_color_step;
            color = room.crt_b_color << 16; break;
        case 3://rg
            color = room.crt_r_color + (room.crt_g_color << 8); break;
        case 4://rb
            color = room.crt_r_color + (room.crt_b_color << 16); break;
        case 5://gb
            color = (room.crt_g_color << 8) + (room.crt_b_color << 16); break;
        case 6://rgb
        default:
            if(room.crt_rgb_color_step > 27){
                room.crt_rgb_color_step -= 17;
            }else{
                return 0;
            }
            color = room.crt_r_color + (room.crt_g_color << 8) + (room.crt_b_color << 16);
    }

    ++room.crt_color_pos;

    return color;
}

}//namespace server
}//namespace bubbles
