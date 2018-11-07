#ifndef BUBBLES_MESSAGES_HPP
#define BUBBLES_MESSAGES_HPP

#include "solid/frame/mprpc/mprpcmessage.hpp"
#include "solid/frame/mprpc/mprpccontext.hpp"
#include "solid/frame/mprpc/mprpcprotocol_serialization_v2.hpp"

#include <vector>
#include <deque>

namespace bubbles{

struct RegisterRequest: solid::frame::mprpc::Message{
    std::string         room_name;
    uint32_t            rgb_color;

    RegisterRequest(){}

    RegisterRequest(
        const std::string &_rroom_name,
        uint32_t _rgb_color
    ): room_name(_rroom_name), rgb_color(_rgb_color){}

    SOLID_PROTOCOL_V2(_s, _rthis, _rctx, _name){
        _s.add(_rthis.room_name, _rctx, "room_name").add(_rthis.rgb_color, _rctx, "rgb_color");
    }
};

struct RegisterResponse: solid::frame::mprpc::Message{
    uint32_t        error;
    uint32_t        rgb_color;
    std::string     message;

    RegisterResponse(){}

    RegisterResponse(
        const RegisterRequest &_rrec, uint32_t _err, const std::string &_msg
    ):solid::frame::mprpc::Message(_rrec), error(_err), rgb_color(0), message(_msg){}

    RegisterResponse(
        const RegisterRequest &_rrec, uint32_t _rgb_color
    ): error(0), rgb_color(_rgb_color){}

    bool success()const{
        return error == 0;
    }

    SOLID_PROTOCOL_V2(_s, _rthis, _rctx, _name){
        _s.add(_rthis.error, _rctx, "error").add(_rthis.rgb_color, _rctx, "rgb_color").add(_rthis.message, _rctx, "message");
    }
};

struct ConnectionId{
    uint32_t    server_idx;
    uint32_t    server_unq;

    uint64_t    connection_idx;
    uint32_t    connection_unq;

    ConnectionId():server_idx(solid::InvalidIndex()), server_unq(solid::InvalidIndex()), connection_idx(solid::InvalidIndex()), connection_unq(solid::InvalidIndex()){}

    bool isValid()const{
        return server_idx != solid::InvalidIndex();
    }

    void clear(){
        server_idx = solid::InvalidIndex();
        connection_idx = solid::InvalidIndex();
    }

    SOLID_PROTOCOL_V2(_s, _rthis, _rctx, _name){
        _s.add(_rthis.server_idx, _rctx, "server_idx").add(_rthis.server_unq, _rctx, "server_unq");
        _s.add(_rthis.connection_idx, _rctx, "connection_idx").add(_rthis.connection_unq, _rctx, "connection_unq");
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

    SOLID_PROTOCOL_V2(_s, _rthis, _rctx, _name){
        _s.add(_rthis.type, _rctx, "type").add(_rthis.flags, _rctx, "flags");
        _s.add(_rthis.x, _rctx, "x").add(_rthis.y, _rctx, "y");
        _s.add(_rthis.data, _rctx, "data").add(_rthis.diff_time_msec, _rctx, "diff_time_msec");
        
    }

    uint16_t    type;
    uint16_t    flags;
    int32_t     x;
    int32_t     y;
    uint64_t    data;
    uint32_t    diff_time_msec;
};

struct EventStub{
    using EventVectorT = std::vector<Event>;

    Event           event;
    ConnectionId    connection_id;
    uint32_t        sender_rgb_color;
    std::string     text;
    EventVectorT    events;

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

    SOLID_PROTOCOL_V2(_s, _rthis, _rctx, _name){
        _s.add(_rthis.event, _rctx, "event");
        _s.add(_rthis.connection_id, _rctx, "connection_id");
        _s.add(_rthis.sender_rgb_color, _rctx, "sender_rgb_color");
        _s.add(_rthis.text, _rctx, "text");
        _s.add(_rthis.events, _rctx, "events");
    }
};

//Push notification - no guarantee and no feedback for delivery
struct EventsNotification: solid::frame::mprpc::Message{

    using EventStubDequeT   = std::deque<EventStub>;

    EventStub           event_stub;
    EventStubDequeT     event_stubs;
    bool                is_init;//not serialized - used on server

    static size_t containerLimit(){
        return 1024;
    }

    EventsNotification():is_init(false){}

    void clear(){
        event_stubs.clear();
        event_stub.clear();
    }

    SOLID_PROTOCOL_V2(_s, _rthis, _rctx, _name){
        const size_t limit_container = _s.limits().container();
        const size_t limit_string = _s.limits().string();
        
        _s.limitContainer(1024, _name);
        _s.limitString(1024, _name);
        _s.add(_rthis.event_stub, _rctx, "event_stub").add(_rthis.event_stubs, _rctx, "event_stubs");
        _s.limitContainer(limit_container, _name);
        _s.limitString(limit_string, _name);
    }
};

//Push notification with feedback for delivery
struct EventsNotificationRequest: EventsNotification{
};

struct EventsNotificationResponse: solid::frame::mprpc::Message{
    uint32_t        error;
    uint32_t        success_count;
    uint32_t        fail_count;
    std::string     message;

    EventsNotificationResponse():error(0){}

    EventsNotificationResponse(
        const EventsNotificationRequest &_req,
        uint32_t _error,
        uint32_t _success_count,
        uint32_t _fail_count,
        const std::string &_rmsg = ""
    ):  solid::frame::mprpc::Message(_req), error(_error),
        success_count(_success_count), fail_count(_fail_count), message(_rmsg){}

    SOLID_PROTOCOL_V2(_s, _rthis, _rctx, _name){
        _s.add(_rthis.error, _rctx, "error");
        _s.add(_rthis.success_count, _rctx, "success_count");
        _s.add(_rthis.fail_count, _rctx, "fail_count");
        _s.add(_rthis.message, _rctx, "message");
    }
};

using ProtocolT = solid::frame::mprpc::serialization_v2::Protocol<uint8_t>;

template <class R>
inline void protocol_setup(R _r, ProtocolT& _rproto)
{
    _rproto.null(static_cast<ProtocolT::TypeIdT>(0));

    _r(_rproto, solid::TypeToType<RegisterRequest>(), 1);
    _r(_rproto, solid::TypeToType<RegisterResponse>(), 2);
    _r(_rproto, solid::TypeToType<EventsNotification>(), 3);
    _r(_rproto, solid::TypeToType<EventsNotificationRequest>(), 4);
    _r(_rproto, solid::TypeToType<EventsNotificationResponse>(), 5);
}


}//namespace bubbles

#endif
