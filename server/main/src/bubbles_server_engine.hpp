#ifndef BUBBLES_SERVER_ENGINE_HPP
#define BUBBLES_SERVER_ENGINE_HPP

#include <ostream>

#include "protocol/bubbles_messages.hpp"
#include "solid/system/error.hpp"

namespace solid{namespace frame{namespace mprpc{
struct ConnectionContext;
}}}

namespace bubbles{
namespace server{

struct ConnectionData;

struct EngineConfiguration{
    EngineConfiguration():connection_max_pending_count(1000){}

    size_t      connection_max_pending_count;
};


class Engine{
public:
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    Engine(const EngineConfiguration &_config);
    ~Engine();


    void onConnectionStart(solid::frame::mprpc::ConnectionContext &_rctx);
    void onConnectionStop(solid::frame::mprpc::ConnectionContext &_rctx);

    void onMessage(
        solid::frame::mprpc::ConnectionContext &_rctx,
        std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
        std::shared_ptr<RegisterRequest> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

    void onMessage(
        solid::frame::mprpc::ConnectionContext &_rctx,
        std::shared_ptr<RegisterResponse> &_rsent_msg_ptr,
        std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

    void onMessage(
        solid::frame::mprpc::ConnectionContext &_rctx,
        std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
        std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

    void onMessage(
        solid::frame::mprpc::ConnectionContext &_rctx,
        std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
        std::shared_ptr<EventsNotificationRequest> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

    void onMessage(
        solid::frame::mprpc::ConnectionContext &_rctx,
        std::shared_ptr<EventsNotificationResponse> &_rsent_msg_ptr,
        std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

    void plotStatistics(std::ostream &);

private:

    uint32_t registerConnection(
        solid::frame::mprpc::ConnectionContext &_rctx,
        ConnectionData &_rcon_data,
        const RegisterRequest &_rreq,
        uint32_t &_rrgb_color
    );

    void unregisterConnection(solid::frame::mprpc::ConnectionContext &_rctx, ConnectionData &_rcon_data);

    uint32_t createNewColor(const size_t _room_index);

    void fetchLastEvents(
        solid::frame::mprpc::ConnectionContext &_rctx, ConnectionData &_rcon_data,
        std::shared_ptr<EventsNotification> &&_rmsg_ptr
    );
private:
    struct Data;
    Data &d;
};

}//namespace server
}//namespace bubbles


#endif
