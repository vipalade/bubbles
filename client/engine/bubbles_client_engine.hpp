#ifndef BUBBLES_CLIENT_ENGINE_HPP
#define BUBBLES_CLIENT_ENGINE_HPP

#include "protocol/bubbles_messages.hpp"
#include "solid/frame/object.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/reactor.hpp"
#include "solid/frame/service.hpp"
#include "solid/utility/dynamicpointer.hpp"
#include <functional>

namespace solid{namespace frame{
namespace mpipc{
    class Service;
}
struct ReactorContext;
}}

namespace bubbles{
namespace client{

using SchedulerT = solid::frame::Scheduler<solid::frame::Reactor>;

struct EngineConfiguration{
    EngineConfiguration(): max_event_queue_size(1024){}
    size_t      max_event_queue_size;
};

class Engine;

struct PlotIterator{
    PlotIterator(PlotIterator &&_plotit);
    PlotIterator();
    ~PlotIterator();

    PlotIterator& operator=(PlotIterator &&_plotit);

    uint32_t rgbColor()const;
    int32_t x()const;
    int32_t y()const;

    const std::string& text()const;

    bool end()const;
    PlotIterator& operator++();

    void clear();

    uint32_t myRgbColor()const{
        return rgb_color;
    }
private:
    friend class Engine;
    PlotIterator(Engine &);
private:
    size_t      pos;
    size_t      plotdq_index;
    uint32_t    rgb_color;
    Engine      *peng;
};

class Engine: public solid::Dynamic<Engine, solid::frame::Object>{
    using ExitFunctionT = std::function<void()>;
    using GuiUpdateFunctionT = std::function<void()>;
    using AutoUpdateFunctionT = std::function<void()>;
public:
    using PointerT = solid::DynamicPointer<Engine>;

    static PointerT create(
        solid::frame::ServiceT &_rsvc, solid::frame::mpipc::Service &_rmpipc,
        const EngineConfiguration &_cfg
    ){
        return PointerT(new Engine(_rsvc, _rmpipc, _cfg));
    }

    solid::ErrorConditionT start(
        SchedulerT &_rsched,
        const std::string &_server_endpoint,
        const std::string &_room_name,
        const bool _auto_pilot = false,
        uint32_t _rgb_color = 0
    );
    
    int canvasWidth()const;
    int canvasHeight()const;
    
    long scaleX(long _x, long _w)const;
    long scaleY(long _y, long _h)const;
    
    long reverseScaleX(long _x, long _w)const;
    long reverseScaleY(long _y, long _h)const;
    
    void pause();
    void resume();

    bool autoPilot()const;

    template <class F>
    void setExitFunction(F _f){
        ExitFunctionT   f{_f};
        doSetExitFunction(std::move(f));
    }

    template <class F>
    void setGuiUpdateFunction(F _f){
        GuiUpdateFunctionT f{_f};
        doSetGuiUpdateFunction(std::move(f));
    }

    template <class F>
    void setAutoUpdateFunction(F _f){
        AutoUpdateFunctionT f{_f};
        doSetAutoUpdateFunction(std::move(f));
    }

    PlotIterator plot();

    void getAutoPosition(int &_rx, int &_ry);

    void moveEvent(int _x, int _y);
    void onConnectionStart(solid::frame::mpipc::ConnectionContext &_rctx);
    void onConnectionStop(solid::frame::mpipc::ConnectionContext &_rctx);

    void onMessage(
        solid::frame::mpipc::ConnectionContext &_rctx,
        std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
        std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

    void onMessage(
        solid::frame::mpipc::ConnectionContext &_rctx,
        std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
        std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

    void onMessage(
        solid::frame::mpipc::ConnectionContext &_rctx,
        std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
        std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
        solid::ErrorConditionT const &_rerror
    );

private:
    Engine(
        solid::frame::ServiceT &_rsvc, solid::frame::mpipc::Service &_rmpipc,
        const EngineConfiguration &_cfg
    );
    ~Engine();

    void onEvent(solid::frame::ReactorContext &_rctx, solid::Event &&_uevent) override;
    void doTrySendEvents();

    void doSetExitFunction(ExitFunctionT &&_uf);
    void doSetGuiUpdateFunction(GuiUpdateFunctionT &&_uf);
    void doSetAutoUpdateFunction(AutoUpdateFunctionT &&_uf);
    void doProcessIncomingNotifications(solid::frame::ReactorContext &_rctx);
    void onAutoPilot(solid::frame::ReactorContext &_rctx);
    void doPause(solid::frame::ReactorContext &_rctx);
    void doResume(solid::frame::ReactorContext &_rctx);
    void doHandleConnectionStop(solid::frame::ReactorContext &_rctx);
private:
    friend struct PlotIterator;
    struct Data;
    Data    &d;
};

}//namespace client
}//namespace bubbles

#endif
