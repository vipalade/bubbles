//
//  engine_glue.cpp
//  bubbles
//
//  Created by Valentin Palade on 07/01/2018.
//  Copyright © 2018 Valentin Palade. All rights reserved.
//

#include "engine_glue.hpp"

#include "solid/frame/manager.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/service.hpp"
#include "solid/frame/reactor.hpp"

#include "solid/frame/aio/aioreactor.hpp"
#include "solid/frame/aio/aioobject.hpp"
#include "solid/frame/aio/aiolistener.hpp"
#include "solid/frame/aio/aiotimer.hpp"
#include "solid/frame/aio/aioresolver.hpp"

#include "solid/frame/aio/openssl/aiosecurecontext.hpp"
#include "solid/frame/aio/openssl/aiosecuresocket.hpp"

#include "solid/frame/mprpc/mprpcservice.hpp"
#include "solid/frame/mprpc/mprpcconfiguration.hpp"
#include "solid/frame/mprpc/mprpcprotocol_serialization_v2.hpp"
#include "solid/frame/mprpc/mprpcsocketstub_openssl.hpp"
#include "solid/frame/mprpc/mprpccompression_snappy.hpp"

#include "protocol/bubbles_messages.hpp"
#include "client/engine/bubbles_client_engine.hpp"


using namespace std;
using namespace solid;

namespace {
    using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;
    using SchedulerT = frame::Scheduler<frame::Reactor>;
    using SecureContextT = frame::aio::openssl::Context;
    using BubblesEnginePointerT = bubbles::client::Engine::PointerT;
    using PlotIteratorT = bubbles::client::PlotIterator;
    
    struct Context {
        Context(
        
        ): done(false), started(false),
        m{}, ipcsvc(m), svc(m), fwp(WorkPoolConfiguration()), resolver(fwp){}
        
        bool                    done;
        bool                    started;
        
        std::string             endpoint;
        std::string             room_name;
        
        AioSchedulerT           aio_sch;
        SchedulerT              sch;
        
        
        frame::Manager          m;
        frame::mprpc::ServiceT  ipcsvc;
        frame::ServiceT         svc;
        FunctionWorkPool        fwp;
        frame::aio::Resolver    resolver;
        BubblesEnginePointerT   engine_ptr;
        PlotIteratorT           plotit;
    }g_ctx;
    
}//namespace


//-----------------------------------------------------------------------------
namespace bubbles{
    namespace client{
        
        struct MessageSetup {
            Engine &engine;

            MessageSetup(Engine &_engine):engine(_engine){}
            
            void operator()(bubbles::ProtocolT& _rprotocol, TypeToType<RegisterRequest> _t2t, const bubbles::ProtocolT::TypeIdT& _rtid)
            {
                Engine &eng = engine;
                auto lambda = [&eng](
                    frame::mprpc::ConnectionContext &_rctx,
                    std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
                    std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
                    ErrorConditionT const &_rerror
                ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerMessage<RegisterRequest>(lambda, _rtid);
            }
            
            void operator()(bubbles::ProtocolT& _rprotocol, TypeToType<RegisterResponse> _t2t, const bubbles::ProtocolT::TypeIdT& _rtid)
            {
                Engine &eng = engine;
                auto lambda = [&eng](
                    frame::mprpc::ConnectionContext &_rctx,
                    std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
                    std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
                    ErrorConditionT const &_rerror
                ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerMessage<RegisterResponse>(lambda, _rtid);
            }
            
            void operator()(bubbles::ProtocolT& _rprotocol, TypeToType<EventsNotificationRequest> _t2t, const bubbles::ProtocolT::TypeIdT& _rtid)
            {
                Engine &eng = engine;
                auto lambda = [&eng](
                    frame::mprpc::ConnectionContext &_rctx,
                    std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
                    std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
                    ErrorConditionT const &_rerror
                ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerMessage<EventsNotificationRequest>(lambda, _rtid);
            }
            
            void operator()(bubbles::ProtocolT& _rprotocol, TypeToType<EventsNotificationResponse> _t2t, const bubbles::ProtocolT::TypeIdT& _rtid)
            {
                Engine &eng = engine;
                auto lambda = [&eng](
                    frame::mprpc::ConnectionContext &_rctx,
                    std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
                    std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
                    ErrorConditionT const &_rerror
                ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerMessage<EventsNotificationResponse>(lambda, _rtid);
            }
            
            template <typename T>
            void operator()(bubbles::ProtocolT& _rprotocol, TypeToType<T> _t2t, const bubbles::ProtocolT::TypeIdT& _rtid)
            {
                Engine &eng = engine;
                auto lambda = [&eng](
                    frame::mprpc::ConnectionContext &_rctx,
                    std::shared_ptr<T> &_rsent_msg_ptr,
                    std::shared_ptr<T> &_rrecv_msg_ptr,
                    ErrorConditionT const &_rerror
                ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerMessage<T>(lambda, _rtid);
            }
        };
        
    }//namespace client
}//namespace bubbles


int engine_start(
    const char *_host,
    const char *_room,
    const int _secure,
    const int _compress,
    const int _auto_pilot,
    const char *_ssl_verify_authority,
    const char *_ssl_client_cert,
    const char *_ssl_client_key,
    void *_pexit_data, ExitCallbackT _exit_fnc,
    void *_pupdate_data, GUIUpdateCallbackT _update_fnc,
    void *_pauto_data, AutoUpdateCallbackT _auto_fnc
){
    
    solid::log_start(std::cerr, {".*:EW"});
    
    g_ctx.endpoint = _host;
    g_ctx.room_name = _room;
    
    string verify_authority_str{_ssl_verify_authority};
    string client_cert_str{_ssl_client_cert};
    string client_key_str{_ssl_client_key};
    
    g_ctx.engine_ptr = bubbles::client::Engine::create(g_ctx.svc, g_ctx.ipcsvc, bubbles::client::EngineConfiguration{});
    
    ErrorConditionT         err;
    auto thr_enter = []() {
        return true;
    };
    
    auto thr_exit = []() {
    };
    
    err = g_ctx.sch.start(thr_enter, thr_exit, 1);
    
    if(err){
        solid_log(generic_logger, Error, "Error starting aio scheduler: "<<err.message());
        return -1;
    }
    
    err = g_ctx.aio_sch.start(1);
    
    if(err){
        solid_log(generic_logger, Error, "Error starting aio scheduler: "<<err.message());
        return -1;
    }
    
    {
        auto                        proto = bubbles::ProtocolT::create();//small limits by default
        frame::mprpc::Configuration cfg(g_ctx.aio_sch, proto);
        
        bubbles::protocol_setup(bubbles::client::MessageSetup(std::ref(*g_ctx.engine_ptr)), *proto);
        
        cfg.client.name_resolve_fnc = frame::mprpc::InternetResolverF(g_ctx.resolver, "4444");
        
        cfg.client.connection_start_state = frame::mprpc::ConnectionState::Passive;
        
        cfg.connection_keepalive_timeout_seconds = 30;
        
        {
            auto connection_stop_lambda = [](frame::mprpc::ConnectionContext &_ctx){
                solid_log(generic_logger, Error, "connection stop: "<<_ctx.error().message()<<" | "<<_ctx.systemError().message());
                g_ctx.engine_ptr->onConnectionStop(_ctx);
            };
            auto connection_start_lambda = [](frame::mprpc::ConnectionContext &_ctx){
                solid_log(generic_logger, Error, "connection start");
                g_ctx.engine_ptr->onConnectionStart(_ctx);
            };
            cfg.connection_stop_fnc = std::move(connection_stop_lambda);
            cfg.client.connection_start_fnc = std::move(connection_start_lambda);
        }
        
        if(_secure){
            //configure OpenSSL:
            frame::mprpc::openssl::setup_client(
                                                cfg,
                                                [verify_authority_str, client_cert_str, client_key_str](frame::aio::openssl::Context &_rctx) -> ErrorCodeT {
                                                    _rctx.addVerifyAuthority(verify_authority_str);
                                                    _rctx.loadCertificate(client_cert_str);
                                                    _rctx.loadPrivateKey(client_key_str);
                                                    return ErrorCodeT();
                                                },
                                                frame::mprpc::openssl::NameCheckSecureStart{"bubbles-server"}
                                                );
        }
        
        if(_compress){
            //configure Snappy compression:
            frame::mprpc::snappy::setup(cfg);
        }
        
        err = g_ctx.ipcsvc.reconfigure(std::move(cfg));
        
        if(err){
            solid_log(generic_logger, Error, "Error starting ipcservice: "<<err.message());
            return -1;
        }
    }
    
    {
        //set the callbacks
        auto exit_closure = [_pexit_data, _exit_fnc](){
            _exit_fnc(_pexit_data);
        };
        auto update_closure = [_pupdate_data, _update_fnc](){
            solid_log(generic_logger, Info, "");
            _update_fnc(_pupdate_data);
        };
        auto auto_closure = [_pauto_data, _auto_fnc](){

            int x,y;
            g_ctx.engine_ptr->getAutoPosition(x, y);
            solid_log(generic_logger, Info, "x = "<<x<<" y = "<<y);
            _auto_fnc(_pauto_data, x, y);
        };
        
        g_ctx.engine_ptr->setExitFunction(exit_closure);
        g_ctx.engine_ptr->setAutoUpdateFunction(auto_closure);
        g_ctx.engine_ptr->setGuiUpdateFunction(update_closure);
        
    }
    solid_log(generic_logger, Info, "starting engine for host: "<<g_ctx.endpoint<<" room: "<<g_ctx.room_name<<" auto_pilot: "<<_auto_pilot);
    err = g_ctx.engine_ptr->start(g_ctx.sch, g_ctx.endpoint, g_ctx.room_name, _auto_pilot == 1);
    
    if(err){
        solid_log(generic_logger, Error, "Error starting engine: "<<err.message());
        return -1;
    }
    g_ctx.started = true;
    solid_log(generic_logger, Info, "Engine started");
    return 0;
}

void engine_move(int _x, int _y){
    g_ctx.engine_ptr->moveEvent(_x, _y);
}

void engine_plot_start(){
    g_ctx.plotit = g_ctx.engine_ptr->plot();
}

void engine_plot_done(){
    g_ctx.plotit.clear();
}

int engine_plot_end(){
    return g_ctx.plotit.end() ? 1 : 0;
}

void engine_plot_next(){
    ++g_ctx.plotit;
}

int engine_plot_x(){
    return g_ctx.plotit.x();
}

int engine_plot_y(){
    return g_ctx.plotit.y();
}

namespace{
    inline int swift_color(uint32_t _c){
        int r = _c & 0xff;
        int g = (_c >> 8) & 0xff;
        int b = (_c >> 16) & 0xff;
        int v = (0xff000000 | (b << 0) | (g << 8) | (r << 16));
        return v;
    }
}


int engine_plot_color(){
    return swift_color(g_ctx.plotit.rgbColor());
}

int engine_plot_my_color(){
    return swift_color(g_ctx.plotit.myRgbColor());
}

int engine_scale_x(int _x, int _w){
    return g_ctx.engine_ptr->scaleX(_x, _w);
}

int engine_scale_y(int _y, int _h){
    return g_ctx.engine_ptr->scaleY(_y, _h);
}

int engine_reverse_scale_x(int _x, int _w){
    return g_ctx.engine_ptr->reverseScaleX(_x, _w);
}

int engine_reverse_scale_y(int _y, int _h){
    return g_ctx.engine_ptr->reverseScaleY(_y, _h);
}
