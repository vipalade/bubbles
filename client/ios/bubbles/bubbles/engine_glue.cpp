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

#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"
#include "solid/frame/mpipc/mpipcprotocol_serialization_v1.hpp"
#include "solid/frame/mpipc/mpipcsocketstub_openssl.hpp"
#include "solid/frame/mpipc/mpipccompression_snappy.hpp"

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
        m{}, ipcsvc(m), svc(m){}
        
        bool                    done;
        bool                    started;
        
        std::string             endpoint;
        std::string             room_name;
        
        AioSchedulerT           aio_sch;
        SchedulerT              sch;
        
        
        frame::Manager          m;
        frame::mpipc::ServiceT  ipcsvc;
        frame::ServiceT         svc;
        frame::aio::Resolver    resolver;
        BubblesEnginePointerT   engine_ptr;
        PlotIteratorT           plotit;
    }g_ctx;
    
    void exit_callback();
    void gui_update_callback();
    void auto_move_callback();
    
}//namespace


//-----------------------------------------------------------------------------
namespace bubbles{
    namespace client{
        
        template <typename T>
        struct MessageSetup;
        
        template<>
        struct MessageSetup<RegisterRequest>{
            Engine &engine;
            
            MessageSetup(Engine &_engine):engine(_engine){}
            
            void operator()(frame::mpipc::serialization_v1::Protocol &_rprotocol, const size_t _protocol_idx, const size_t _message_idx){
                Engine &eng = engine;
                auto lambda = [&eng](
                                     frame::mpipc::ConnectionContext &_rctx,
                                     std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
                                     std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
                                     ErrorConditionT const &_rerror
                                     ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerType<RegisterRequest>(lambda, _protocol_idx, _message_idx);
            }
        };
        
        template<>
        struct MessageSetup<RegisterResponse>{
            Engine &engine;
            
            MessageSetup(Engine &_engine):engine(_engine){}
            
            void operator()(frame::mpipc::serialization_v1::Protocol &_rprotocol, const size_t _protocol_idx, const size_t _message_idx){
                Engine &eng = engine;
                auto lambda = [&eng](
                                     frame::mpipc::ConnectionContext &_rctx,
                                     std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
                                     std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
                                     ErrorConditionT const &_rerror
                                     ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerType<RegisterResponse>(lambda, _protocol_idx, _message_idx);
            }
        };
        
        template<>
        struct MessageSetup<EventsNotificationRequest>{
            Engine &engine;
            
            MessageSetup(Engine &_engine):engine(_engine){}
            
            void operator()(frame::mpipc::serialization_v1::Protocol &_rprotocol, const size_t _protocol_idx, const size_t _message_idx){
                Engine &eng = engine;
                auto lambda = [&eng](
                                     frame::mpipc::ConnectionContext &_rctx,
                                     std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
                                     std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
                                     ErrorConditionT const &_rerror
                                     ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerType<EventsNotificationRequest>(lambda, _protocol_idx, _message_idx);
            }
        };
        
        template<>
        struct MessageSetup<EventsNotificationResponse>{
            Engine &engine;
            
            MessageSetup(Engine &_engine):engine(_engine){}
            
            void operator()(frame::mpipc::serialization_v1::Protocol &_rprotocol, const size_t _protocol_idx, const size_t _message_idx){
                Engine &eng = engine;
                auto lambda = [&eng](
                                     frame::mpipc::ConnectionContext &_rctx,
                                     std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
                                     std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
                                     ErrorConditionT const &_rerror
                                     ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerType<EventsNotificationResponse>(lambda, _protocol_idx, _message_idx);
            }
        };
        
        template <typename M>
        struct MessageSetup{
            Engine &engine;
            
            MessageSetup(Engine &_engine):engine(_engine){}
            
            void operator()(frame::mpipc::serialization_v1::Protocol &_rprotocol, const size_t _protocol_idx, const size_t _message_idx){
                Engine &eng = engine;
                auto lambda = [&eng](
                                     frame::mpipc::ConnectionContext &_rctx,
                                     std::shared_ptr<M> &_rsent_msg_ptr,
                                     std::shared_ptr<M> &_rrecv_msg_ptr,
                                     ErrorConditionT const &_rerror
                                     ){
                    eng.onMessage(_rctx, _rsent_msg_ptr, _rrecv_msg_ptr, _rerror);
                };
                _rprotocol.registerType<M>(lambda, _protocol_idx, _message_idx);
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
    const char *_ssl_client_key
){
    
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
        cout<<"Error starting aio scheduler: "<<err.message()<<endl;
        return -1;
    }
    
    err = g_ctx.aio_sch.start(1);
    
    if(err){
        cout<<"Error starting aio scheduler: "<<err.message()<<endl;
        return -1;
    }
    
    err = g_ctx.resolver.start(1);
    
    if(err){
        cout<<"Error starting aio resolver: "<<err.message()<<endl;
        return -1;
    }
    
    {
        auto                        proto = frame::mpipc::serialization_v1::Protocol::create(serialization::binary::Limits(256, 128, 0));//small limits by default
        frame::mpipc::Configuration cfg(g_ctx.aio_sch, proto);
        
        bubbles::ProtoSpecT::setup<bubbles::client::MessageSetup>(*proto, 0, std::ref(*g_ctx.engine_ptr));
        
        cfg.client.name_resolve_fnc = frame::mpipc::InternetResolverF(g_ctx.resolver, "4444");
        
        cfg.client.connection_start_state = frame::mpipc::ConnectionState::Passive;
        
        cfg.connection_keepalive_timeout_seconds = 30;
        
        {
            auto connection_stop_lambda = [](frame::mpipc::ConnectionContext &_ctx){
                cout<<"connection stop"<<endl;
                g_ctx.engine_ptr->onConnectionStop(_ctx);
            };
            auto connection_start_lambda = [](frame::mpipc::ConnectionContext &_ctx){
                cout<<"connection start"<<endl;
                g_ctx.engine_ptr->onConnectionStart(_ctx);
            };
            cfg.connection_stop_fnc = connection_stop_lambda;
            cfg.client.connection_start_fnc = connection_start_lambda;
        }
        
        if(_secure){
            //configure OpenSSL:
            frame::mpipc::openssl::setup_client(
                                                cfg,
                                                [verify_authority_str, client_cert_str, client_key_str](frame::aio::openssl::Context &_rctx) -> ErrorCodeT {
                                                    _rctx.addVerifyAuthority(verify_authority_str);
                                                    _rctx.loadCertificate(client_cert_str);
                                                    _rctx.loadPrivateKey(client_key_str);
                                                    return ErrorCodeT();
                                                },
                                                frame::mpipc::openssl::NameCheckSecureStart{"bubbles-server"}
                                                );
        }
        
        if(_compress){
            //configure Snappy compression:
            frame::mpipc::snappy::setup(cfg);
        }
        
        err = g_ctx.ipcsvc.reconfigure(std::move(cfg));
        
        if(err){
            cout<<"Error starting ipcservice: "<<err.message()<<endl;
            return -1;
        }
    }
    
    {
        //set the callbacks
        
        g_ctx.engine_ptr->setExitFunction(exit_callback);
        g_ctx.engine_ptr->setAutoUpdateFunction(auto_move_callback);
        g_ctx.engine_ptr->setGuiUpdateFunction(gui_update_callback);
        
    }
    
    err = g_ctx.engine_ptr->start(g_ctx.sch, g_ctx.endpoint, g_ctx.room_name, _auto_pilot == 1);
    
    if(err){
        cout<<"Error starting engine: "<<err.message()<<endl;
        return -1;
    }
    g_ctx.started = true;
    return 0;
}

namespace{

void exit_callback(){}
void gui_update_callback(){}
void auto_move_callback(){}
    
}//namespace
