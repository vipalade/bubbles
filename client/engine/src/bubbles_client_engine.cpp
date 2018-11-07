#include "client/engine/bubbles_client_engine.hpp"
#include "solid/frame/mprpc/mprpcservice.hpp"
#include "solid/frame/mprpc/mprpcconfiguration.hpp"

#include "solid/frame/timer.hpp"
#include "solid/frame/reactorcontext.hpp"
#include "solid/system/log.hpp"
#include "solid/system/cassert.hpp"
#include "solid/utility/any.hpp"
#include "solid/utility/event.hpp"

#include <queue>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <random>



using namespace solid;
using namespace std;

namespace bubbles{
namespace client{

using EventQueueT = queue<Event>;

using EventsNotificationDequeT = deque<std::shared_ptr<EventsNotification>>;

struct PlotStub{
    uint32_t    rgb_color;
    int32_t     x;
    int32_t     y;
    string      text;
    size_t      pending_pos;
    bool        ploted;
};

using PlotStubDequeT = deque<PlotStub>;
using EventStubDequeT = deque<EventStub>;

using AutoPairT = std::pair<int, int>;
using AutoAtomicPairT = std::pair<atomic<int>, atomic<int>>;
using AutoQueueT = std::queue<AutoPairT>;
using AtomicBoolT = atomic<bool>;

enum class Events {
    ConnectionStopped
};

const EventCategory<Events> event_category{
    "bubbles::client::engine::event_category",
    [](const Events _evt) {
        switch (_evt) {
        case Events::ConnectionStopped:
            return "ConnectionStopped";
        default:
            return "unknown";
        }
    }};



struct Engine::Data{
    static const int canvas_width = 7680;
    static const int canvas_height = 7680;//use the 8K width
    
    Data(
        solid::frame::ServiceT &_rsvc,
        solid::frame::mprpc::Service &_rmprpc,
        const EngineConfiguration &_cfg,
        const frame::ObjectProxy &_proxy
    ):  rmprpc(_rmprpc), service(_rsvc), cfg(_cfg), push_eventq_idx(0), pop_eventq_idx(1),
        push_messagedq_idx(0), pop_messagedq_idx(1), read_plotdq_idx(0), write_plotdq_idx(1), read_plotdq_count(0),
        discarded_on_push(false), rgb_color(0), auto_pilot(false), timer(_proxy), auto_timer(_proxy),
        auto_crt_w(canvas_width/2), auto_crt_h(canvas_height/2), auto_mod_w(0), auto_mod_h(0), auto_frame_changed(false), auto_plot_done(true),
        auto_plot_idx(0), auto_fill_idx(1),
        auto_dist_x(-auto_crt_w, auto_crt_w), auto_dist_y(-auto_crt_h, auto_crt_h),
        auto_dist_steps(1, 100), paused(false)
    {
        auto_plot[0].first = 0;
        auto_plot[0].second = 0;
        auto_plot[1].first = 0;
        auto_plot[1].second = 0;
    }

    void discardPopEventQ(){
        EventQueueT &eq = eventq[pop_eventq_idx];
        while(eq.size()) eq.pop();
    }

    frame::mprpc::Service                   &rmprpc;

    frame::ServiceT                         &service;
    string                                  server_endpoint;
    string                                  room_name;
    EventQueueT                             eventq[2];
    EngineConfiguration                     cfg;

    size_t                                  push_eventq_idx;
    size_t                                  pop_eventq_idx;

    size_t                                  push_messagedq_idx;
    size_t                                  pop_messagedq_idx;

    size_t                                  read_plotdq_idx;
    size_t                                  write_plotdq_idx;
    size_t                                  read_plotdq_count;

    bool                                    discarded_on_push;
    uint32_t                                rgb_color;
    bool                                    auto_pilot;
    Event                                   last_event;
    std::shared_ptr<EventsNotification>     events_message_ptr;//shared_ptr beacause mprpc sendMessage uses shared_ptr
    std::shared_ptr<EventsNotification>     tmp_events_message_ptr;

    //all functions must be called on the engine's thread
    ExitFunctionT                           exit_function;
    GuiUpdateFunctionT                      gui_update_function;
    AutoUpdateFunctionT                     auto_update_function;

    EventsNotificationDequeT                messagedq[2];
    PlotStubDequeT                          plotdq[2];
    EventStubDequeT                         event_stubdq;
    mutex                                   mtx;
    condition_variable                      cnd;
    frame::SteadyTimer                      timer;
    frame::SteadyTimer                      auto_timer;

    int                                     auto_crt_w;
    int                                     auto_crt_h;
    int                                     auto_mod_w;
    int                                     auto_mod_h;

    std::atomic<bool>                       auto_frame_changed;
    std::atomic<bool>                       auto_plot_done;
    std::atomic<uint16_t>                   auto_plot_idx;
    uint16_t                                auto_fill_idx;
    AutoAtomicPairT                         auto_plot[2];
    random_device                           auto_rd;
    std::uniform_int_distribution<int>      auto_dist_x;
    std::uniform_int_distribution<int>      auto_dist_y;
    std::uniform_int_distribution<int>      auto_dist_steps;
    AutoQueueT                              auto_q;
    AtomicBoolT                             paused;
    frame::mprpc::RecipientId               mprpc_recipient;
};


//=============================================================================
PlotIterator::PlotIterator(PlotIterator &&_plotit):peng(_plotit.peng){
    plotdq_index = _plotit.plotdq_index;
    pos = _plotit.pos;
    _plotit.peng = nullptr;
    rgb_color = _plotit.rgb_color;
}

PlotIterator::PlotIterator():peng(nullptr){}

PlotIterator::~PlotIterator(){
    clear();
}

void PlotIterator::clear(){
    if(peng){
        std::unique_lock<std::mutex>    lock(peng->d.mtx);
        --peng->d.read_plotdq_count;
        if(peng->d.read_plotdq_count == 0){
            peng->d.cnd.notify_one();
        }
        peng = nullptr;
    }
}

PlotIterator& PlotIterator::operator=(PlotIterator &&_plotit){
    peng = _plotit.peng;
    plotdq_index = _plotit.plotdq_index;
    pos = _plotit.pos;
    _plotit.peng = nullptr;
    rgb_color = _plotit.rgb_color;
    return *this;
}
uint32_t PlotIterator::rgbColor()const{
    return peng->d.plotdq[plotdq_index][pos].rgb_color;
}

int32_t PlotIterator::x()const{
    return peng->d.plotdq[plotdq_index][pos].x;
}
int32_t PlotIterator::y()const{
    return peng->d.plotdq[plotdq_index][pos].y;
}

const std::string& PlotIterator::text()const{
    return peng->d.plotdq[plotdq_index][pos].text;
}

bool PlotIterator::end()const{
    return peng->d.plotdq[plotdq_index].size() == pos;
}

PlotIterator& PlotIterator::operator++(){
    ++pos;
    return *this;
}

PlotIterator::PlotIterator(Engine &_reng):peng(&_reng){
    pos = 0;
    std::unique_lock<std::mutex>    lock(peng->d.mtx);
    plotdq_index = peng->d.read_plotdq_idx;
    ++peng->d.read_plotdq_count;
    rgb_color = peng->d.rgb_color;
}

//=============================================================================
Engine::Engine(
    solid::frame::ServiceT &_rsvc, solid::frame::mprpc::Service &_rmprpc,
    const EngineConfiguration &_cfg
):d(*(new Data(_rsvc, _rmprpc, _cfg, proxy()))){}

Engine::~Engine(){
    delete &d;
}

solid::ErrorConditionT Engine::start(
    SchedulerT &_rsched,
    const std::string &_server_endpoint,
    const std::string &_room_name,
    const bool _auto_pilot,
    uint32_t _rgb_color
){
    solid_log(generic_logger, Info, "");
    solid::ErrorConditionT                  err;

    if(!this->isRunning()){
        solid::DynamicPointer<frame::Object>    objptr(this);//its save - pointer count is kept by this pointer
        d.server_endpoint = _server_endpoint;
        d.room_name = _room_name;
        d.auto_pilot = _auto_pilot;
        d.rgb_color = _rgb_color;

        _rsched.startObject(objptr, d.service, solid::generic_event_category.event(solid::GenericEvents::Start), err);
    }
    return err;
}

int Engine::canvasWidth()const{
    return d.canvas_width;
}

int Engine::canvasHeight()const{
    return d.canvas_height;
}

long Engine::scaleX(long _x, long _w)const{
    return (((_x + d.canvas_width/2) * _w) / d.canvas_width) - _w/2;
}

long Engine::scaleY(long _y, long _h)const{
    return (((_y + d.canvas_height/2) *_h) / d.canvas_height) - _h/2;
}


long Engine::reverseScaleX(long _x, long _w)const{
    return (((_x + _w/2) * d.canvas_width) / _w) - d.canvas_width/2;
}

long Engine::reverseScaleY(long _y, long _h)const{
    return (((_y + _h/2) * d.canvas_height) / _h) - d.canvas_height/2;
}

void Engine::pause(){
    d.paused = true;
    d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Pause));
}

void Engine::resume(){
    if(d.paused){
        d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Resume));
    }
}

PlotIterator Engine::plot(){
    return PlotIterator(*this);
}

bool Engine::autoPilot()const{
    return d.auto_pilot;
}

void Engine::moveEvent(int _x, int _y){

    solid_log(generic_logger, Info, _x<<':'<<_y);
    bool notify_engine = false;
    {
        std::unique_lock<std::mutex>    lock(d.mtx);
        EventQueueT                     &reventq = d.eventq[d.push_eventq_idx];

        if((reventq.size() + 1) == d.cfg.max_event_queue_size){
            reventq.pop();
            d.discarded_on_push = true;
        }
        reventq.push(Event());
        reventq.back().type = Event::PointerMove;
        reventq.back().x = _x;
        reventq.back().y = _y;
        notify_engine = (reventq.size() == 1);
    }
    if(notify_engine){
        d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Raise));
    }
}

void Engine::onEvent(frame::ReactorContext &_rctx, solid::Event &&_uevent) /*override*/{
    solid_log(generic_logger, Info, " event = "<<_uevent);
    if(generic_event_category.event(GenericEvents::Start) == _uevent){

        d.events_message_ptr = std::make_shared<EventsNotification>();
        if(d.auto_pilot){
            onAutoPilot(_rctx);
        }
    }else if(generic_event_category.event(GenericEvents::Kill) == _uevent){
        d.paused = true;
        postStop(_rctx);
    }else if(generic_event_category.event(GenericEvents::Raise) == _uevent){
        doTrySendEvents();
    }else if(generic_event_category.event(GenericEvents::Message) == _uevent){
        doProcessIncomingNotifications(_rctx);
    }else if(generic_event_category.event(GenericEvents::Stop) == _uevent){
        d.exit_function();
    }else if(generic_event_category.event(GenericEvents::Update) == _uevent){
        if(!d.paused){
            d.gui_update_function();
        }
    }else if(generic_event_category.event(GenericEvents::Pause) == _uevent){
        doPause(_rctx);
    }else if(generic_event_category.event(GenericEvents::Resume) == _uevent){
        doResume(_rctx);
    }else if(event_category.event(Events::ConnectionStopped) == _uevent){
        doHandleConnectionStop(_rctx);
    }
}

void Engine::doPause(solid::frame::ReactorContext &_rctx){
    auto lambda = [](solid::frame::mprpc::ConnectionContext &_rctx){};
    d.rmprpc.forceCloseConnectionPool(d.mprpc_recipient, lambda);
}

void Engine::doResume(solid::frame::ReactorContext &_rctx){
    d.paused = false;
    if(d.events_message_ptr){
        d.events_message_ptr->event_stub.event = d.last_event;
        std::shared_ptr<EventsNotification> tmp_ptr{std::move(d.events_message_ptr)};
        d.rmprpc.sendMessage(d.server_endpoint.c_str(), tmp_ptr);
    }
    //clear all events
    auto& rplotdq = d.plotdq[d.write_plotdq_idx];

    rplotdq.clear();
    d.event_stubdq.clear();

    if(autoPilot()){
        this->post(_rctx, [this](solid::frame::ReactorContext &_rctx, solid::Event&&){onAutoPilot(_rctx);});
    }
}

void Engine::doHandleConnectionStop(solid::frame::ReactorContext &_rctx){
    if(d.events_message_ptr && !d.paused){
        solid_log(generic_logger, Info, "");
        //connection stopped and there is no activity to send, resend the last event
        d.events_message_ptr->event_stub.event = d.last_event;

        std::shared_ptr<EventsNotification> tmp_ptr{std::move(d.events_message_ptr)};
        solid::ErrorConditionT  err = d.rmprpc.sendMessage(d.server_endpoint.c_str(), tmp_ptr);
        if(err){
            solid_log(generic_logger, Error, ""<< " sendMessage error: "<<err.message());
        }
        
        //clear all events
        auto& rplotdq = d.plotdq[d.write_plotdq_idx];

        rplotdq.clear();
        d.event_stubdq.clear();
    }
}

void Engine::getAutoPosition(int &_rx, int &_ry){
    const size_t idx = d.auto_plot_idx;
    _rx = d.auto_plot[idx].first;
    _ry = d.auto_plot[idx].second;
    d.auto_plot_done = true;
}

void Engine::onAutoPilot(solid::frame::ReactorContext &_rctx){
    const bool frame_changed = d.auto_frame_changed.exchange(false);

    if(frame_changed){
        std::unique_lock<std::mutex> lock(d.mtx);
        d.auto_crt_w = d.auto_mod_w;
        d.auto_crt_h = d.auto_mod_h;
    }
    if(frame_changed){
        d.auto_dist_x = std::uniform_int_distribution<>(-(d.auto_crt_w), d.auto_crt_w);
        d.auto_dist_y = std::uniform_int_distribution<>(-(d.auto_crt_h), d.auto_crt_h);
    }
    if(d.paused){
    }else if(d.auto_q.size()){
        if(d.auto_plot_done){
            d.auto_plot[d.auto_fill_idx] = d.auto_q.front();
            d.auto_q.pop();
            d.auto_fill_idx = d.auto_plot_idx.exchange(d.auto_fill_idx);
            d.auto_plot_done = false;
            d.auto_timer.waitUntil(_rctx, _rctx.steadyTime() + std::chrono::milliseconds(100), [this](solid::frame::ReactorContext &_rctx){onAutoPilot(_rctx);});
            d.auto_update_function();
        }else{
            this->post(_rctx, [this](solid::frame::ReactorContext &_rctx, solid::Event&&){onAutoPilot(_rctx);});
        }
    }else{
        //refill auto_q
        std::mt19937 gen(d.auto_rd());
        float new_x = d.auto_dist_x(gen);
        float new_y = d.auto_dist_y(gen);
        int   new_s = d.auto_dist_steps(gen);

        float old_x = d.auto_plot[d.auto_plot_idx].first;
        float old_y = d.auto_plot[d.auto_plot_idx].second;

        float dis_x = abs(new_x - old_x);
        float dis_y = abs(new_y - old_y);

        float slope = (new_y - old_y)/(new_x - old_x);

        //solid_log(generic_logger, Info, old_x<<':'<<old_y<<" -> "<<new_x<<':'<<new_y<<" in "<<new_s<<" steps");

        if(dis_x < dis_y){
            //go by y axis
            float step = (new_y - old_y) / new_s;
            float crt_y = old_y + step;
            for(int i = 0; i < new_s; ++i, crt_y += step){
                float crt_x = ((crt_y - old_y) + old_x * slope)/slope;
                int icrt_x = crt_x;
                int icrt_y = crt_y;
                //solid_log(generic_logger, Info, crt_x<<':'<<crt_y<<" "<<icrt_x<<':'<<icrt_y);
                if(d.auto_q.empty() || (d.auto_q.back().first != icrt_x || d.auto_q.back().second != icrt_y)){

                    d.auto_q.push(AutoPairT(icrt_x, icrt_y));
                }
            }
        }else{
            //go by x axis
            float step = (new_x - old_x) / new_s;
            float crt_x = old_x + step;
            for(int i = 0; i < new_s; ++i, crt_x += step){
                float crt_y = (slope*(crt_x - old_x)) + old_y;
                int icrt_x = crt_x;
                int icrt_y = crt_y;

                //solid_log(generic_logger, Info, crt_x<<':'<<crt_y<<" "<<icrt_x<<':'<<icrt_y);

                if(d.auto_q.empty() || (d.auto_q.back().first != icrt_x || d.auto_q.back().second != icrt_y)){
                    d.auto_q.push(AutoPairT(icrt_x, icrt_y));
                }
            }
        }

        this->post(_rctx, [this](solid::frame::ReactorContext &_rctx, solid::Event&&){onAutoPilot(_rctx);});
    }
}

void Engine::doProcessIncomingNotifications(solid::frame::ReactorContext &_rctx){
    solid_log(generic_logger, Info, "");
    {
        std::unique_lock<std::mutex> lock(d.mtx);
        swap(d.pop_messagedq_idx, d.push_messagedq_idx);
    }

    EventsNotificationDequeT    &rmessagedq{d.messagedq[d.pop_messagedq_idx]};

    //put all the event stubs in a queue
    for(auto& msg_ptr:rmessagedq){
        solid_log(generic_logger, Info, " event_stub with color ("<<msg_ptr->event_stub.sender_rgb_color<<") main event "<<msg_ptr->event_stub.event.x<<":"<<msg_ptr->event_stub.event.y<<" and other "<<msg_ptr->event_stub.events.size()<<" events");
        d.event_stubdq.push_back(std::move(msg_ptr->event_stub));
        for(auto &e_s: msg_ptr->event_stubs){
            solid_log(generic_logger, Info, " event_stub with color ("<<e_s.sender_rgb_color<<") main event "<<e_s.event.x<<":"<<e_s.event.y<<" and other "<<e_s.events.size()<<" events");
            d.event_stubdq.push_back(std::move(e_s));
        }
    }

    rmessagedq.clear();

    auto& rplotdq = d.plotdq[d.write_plotdq_idx];

    const size_t qsz = d.event_stubdq.size();
    for(size_t i = 0; i < qsz; ++i){
        auto&       revent_stub = d.event_stubdq.front();

        bool        drop_event_stub = false;

        auto bin_srch_ret = solid::binary_search(
            rplotdq.begin(), rplotdq.end(), revent_stub.sender_rgb_color,
            [](const PlotStub &_rps, const uint32_t _key){
                if(_key < _rps.rgb_color) return -1;
                if(_key > _rps.rgb_color) return 1;
                return 0;
            }
        );

        if(bin_srch_ret.first){//entry found

            PlotStub &rps = rplotdq[bin_srch_ret.second];

            if(rps.ploted){

                //we can change to a new event
                if(rps.pending_pos < (revent_stub.events.size() + 1)){
                    Event &revt = rps.pending_pos == 0 ? revent_stub.event : revent_stub.events[rps.pending_pos];

                    if(revt.type == Event::Unknown){//entry must be erased
                        rplotdq.erase(rplotdq.begin() + bin_srch_ret.second);
                        drop_event_stub = true;
                    }else{
                        rps.ploted = false;
                        rps.text = revent_stub.text;
                        rps.x = revt.x;
                        rps.y = revt.y;
                        ++rps.pending_pos;
                        if(rps.pending_pos >= (revent_stub.events.size() + 1)){
                            rps.pending_pos = 0;
                            drop_event_stub = true;
                        }
                    }
                }
            }
        }else{//entry not found - insert it
            Event &revt = revent_stub.event;
            if(revt.type != Event::Unknown){//entry must be erased
                PlotStub &rps = *(rplotdq.insert(rplotdq.begin() + bin_srch_ret.second, PlotStub{}));
                rps.rgb_color = revent_stub.sender_rgb_color;
                rps.text = revent_stub.text;
                rps.x = revt.x;
                rps.y = revt.y;
                rps.pending_pos = 1;
                rps.ploted = false;
                if(rps.pending_pos >= (revent_stub.events.size() + 1)){
                    rps.pending_pos = 0;
                    drop_event_stub = true;
                }
            }else{
                drop_event_stub = true;
            }
        }

        if(!drop_event_stub){
            d.event_stubdq.push_back(std::move(revent_stub));
        }

        d.event_stubdq.pop_front();
    }

    //now we need to make visible the d.plotdq we've just modified
    {
        std::unique_lock<std::mutex> lock(d.mtx);
        while(d.read_plotdq_count){
            d.cnd.wait(lock);
        }
        swap(d.write_plotdq_idx, d.read_plotdq_idx);
    }

    d.plotdq[d.write_plotdq_idx] = d.plotdq[d.read_plotdq_idx];

    for(auto& plot_stub: d.plotdq[d.write_plotdq_idx]){
        plot_stub.ploted = true;
    }
    if(!d.paused){
        //update gui
        d.gui_update_function();

        if(d.event_stubdq.size()){
            d.timer.waitUntil(_rctx, _rctx.steadyTime() + std::chrono::milliseconds(20), [this](solid::frame::ReactorContext &_rctx){doProcessIncomingNotifications(_rctx);});
        }
    }
}

void Engine::doTrySendEvents(){
    solid_log(generic_logger, Info, "");
    size_t      pop_eventq_idx = 0;
    {
        if(d.tmp_events_message_ptr){
            solid_assert(!d.events_message_ptr);
            d.events_message_ptr = std::move(d.tmp_events_message_ptr);
        }
        
        
        if(d.events_message_ptr && !d.paused){
            std::unique_lock<std::mutex> lock(d.mtx);
            //the message is ready to fill - see which of the eventq we should use
            if(d.discarded_on_push){
                //the push eventq is already rotating, drop what remains on pop_eventq
                d.discardPopEventQ();
                d.discarded_on_push = false;
                swap(d.pop_eventq_idx, d.push_eventq_idx);
                pop_eventq_idx = d.pop_eventq_idx;
            }else if(d.eventq[d.pop_eventq_idx].size()){
                //still have envents on popq
                pop_eventq_idx = d.pop_eventq_idx;
            }else if(d.eventq[d.push_eventq_idx].size()){
                //have events on pushq
                swap(d.pop_eventq_idx, d.push_eventq_idx);
                pop_eventq_idx = d.pop_eventq_idx;
            }else{
                pop_eventq_idx = -1;
            }
        }else{
            pop_eventq_idx = -1;
        }
    }
    if(pop_eventq_idx < 2 && d.events_message_ptr){
        //we can fill events_message_ptr and send it to server
        EventQueueT &reventq = d.eventq[pop_eventq_idx];

        d.last_event = reventq.back();

        d.events_message_ptr->event_stub.event = reventq.front();
        reventq.pop();

        while(reventq.size() && d.events_message_ptr->event_stub.events.size() < 1000){
            d.events_message_ptr->event_stub.events.push_back(reventq.front());
            reventq.pop();
        }

        std::shared_ptr<EventsNotification> tmp_ptr{std::move(d.events_message_ptr)};
        solid::ErrorConditionT  err = d.rmprpc.sendMessage(d.server_endpoint.c_str(), tmp_ptr);
        if(err){
            solid_log(generic_logger, Error, ""<< " sendMessage error: "<<err.message());
        }
    }
}

void Engine::onConnectionStart(solid::frame::mprpc::ConnectionContext &_rctx){
    solid_log(generic_logger, Info, _rctx.recipientId());
    if(!d.paused){
        d.mprpc_recipient = _rctx.recipientId();
        auto msg_ptr = std::make_shared<RegisterRequest>(d.room_name, d.rgb_color);
        solid::ErrorConditionT  err;
        solid_check(!(err = _rctx.service().sendMessage(_rctx.recipientId(), msg_ptr, {frame::mprpc::MessageFlagsE::WaitResponse})), "failed send message: "<<err.message());
    }else{
        auto lambda = [](solid::frame::mprpc::ConnectionContext &_rctx){};
        d.rmprpc.forceCloseConnectionPool(_rctx.recipientId(), lambda);
    }
}

void Engine::onConnectionStop(solid::frame::mprpc::ConnectionContext &_rctx){
    solid_log(generic_logger, Info, _rctx.recipientId()<<' '<<_rctx.error().message()<<' '<<d.events_message_ptr);
    if(!d.paused){
        d.service.manager().notify(d.service.manager().id(*this), event_category.event(Events::ConnectionStopped));
    }
}

void Engine::doSetExitFunction(ExitFunctionT &&_uf){
    d.exit_function = std::move(_uf);
}
void Engine::doSetGuiUpdateFunction(GuiUpdateFunctionT &&_uf){
    d.gui_update_function = std::move(_uf);
}
void Engine::doSetAutoUpdateFunction(AutoUpdateFunctionT &&_uf){
    d.auto_update_function = _uf;
}

void Engine::onMessage(
    solid::frame::mprpc::ConnectionContext &_rctx,
    std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
    std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){

    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());

    if(d.paused) return;

    if(_rrecv_msg_ptr && _rrecv_msg_ptr->success()){
        d.rgb_color = _rrecv_msg_ptr->rgb_color;

        solid_log(generic_logger, Info, _rctx.recipientId()<<" MY COLOR: "<<d.rgb_color);
        //clear all events
        
        //NOTE: we cannot clear here - we must move to Engine's thread
        //auto& rplotdq = d.plotdq[d.write_plotdq_idx];

        //rplotdq.clear();
        //d.event_stubdq.clear();

        //after activation, the mprpc will start sending pending EventsNotification messages
        _rctx.service().connectionNotifyEnterActiveState(_rctx.recipientId());
        d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Resume));
    }else if(_rrecv_msg_ptr){
        //failed registering the connection
        solid_log(generic_logger, Error, _rctx.recipientId()<<" Connection registration failed because ["<<_rrecv_msg_ptr->message<<"]. Exiting");
        d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Stop));
    }
}

void Engine::onMessage(
    solid::frame::mprpc::ConnectionContext &_rctx,
    std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
    std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){
    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());
    if(_rrecv_msg_ptr){
        bool notify = false;
        {
            std::unique_lock<std::mutex>    lock(d.mtx);
            EventsNotificationDequeT        &rmessagedq = d.messagedq[d.push_messagedq_idx];

            rmessagedq.push_back(std::move(_rrecv_msg_ptr));
            notify = (rmessagedq.size() == 1);
        }

        if(notify){
            d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Message));
        }
    }else if(_rsent_msg_ptr){
        _rsent_msg_ptr->clear();
        solid_assert(!d.tmp_events_message_ptr);
        d.tmp_events_message_ptr = std::move(_rsent_msg_ptr);
        d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Raise));
    }
}

void Engine::onMessage(
    solid::frame::mprpc::ConnectionContext &_rctx,
    std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
    std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
    solid::ErrorConditionT const &_rerror
){
    solid_log(generic_logger, Info, _rctx.recipientId()<<" error: "<<_rerror.message());
}


}//namespace client
}//namespace bubbles
