#include <jni.h>
#include <string>
#include <android/log.h>
#include <cassert>

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

#include "client/engine/bubbles_client_engine.hpp"
#include "protocol/bubbles_messages.hpp"

using namespace std;
using namespace solid;

// Android log function wrappers
static const char* kTAG = "bubbles-native";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

namespace {
    typedef frame::Scheduler<frame::aio::Reactor>	AioSchedulerT;
    typedef frame::Scheduler<frame::Reactor>        SchedulerT;
    typedef frame::aio::openssl::Context			SecureContextT;
    using BubblesEnginePointerT = bubbles::client::Engine::PointerT;
    struct Context {
        Context(

        ):  vm(nullptr), done(false), started(false), echoActivityClz(nullptr), echoActivityObj(nullptr),
            m{}, ipcsvc(m), svc(m){}

        JavaVM                  *vm;
        bool                    done;
        bool                    started;
        jclass                  echoActivityClz;
        jobject                 echoActivityObj;

        std::string             endpoint;
        std::string             room_name;

        AioSchedulerT			aio_sch;
        SchedulerT              sch;


        frame::Manager			m;
        frame::mpipc::ServiceT	ipcsvc;
        frame::ServiceT         svc;
        frame::aio::Resolver	resolver;
        BubblesEnginePointerT	engine_ptr;
    }g_ctx;

    thread_local JNIEnv                  *local_java_env = nullptr;
    thread_local jmethodID               local_java_bubbles_refresh_method_id;

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

//-----------------------------------------------------------------------------

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;

    g_ctx.vm = vm;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // JNI version not supported.
    }

    LOGI("native load");
    return  JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    g_ctx.done = true;
    LOGI("native unload");
}


extern "C"
jboolean Java_com_example_vapa_bubbles_BubblesActivity_nativeStart(
        JNIEnv *env,
        jobject _this, jstring _endpoint, jstring _room, jboolean _secure, jboolean _auto_pilot,
        jstring _verify_authority, jstring _client_cert, jstring _client_key
){
    LOGI("native start");

    string verify_authority_str;
    string client_cert_str;
    string client_key_str;

    {
        const char *utf_ep = env->GetStringUTFChars(_endpoint, nullptr);

        g_ctx.endpoint = utf_ep;
        env->ReleaseStringUTFChars(_endpoint, utf_ep);
    }
    {
        const char *utf_ep = env->GetStringUTFChars(_room, nullptr);

        g_ctx.room_name = utf_ep;
        env->ReleaseStringUTFChars(_endpoint, utf_ep);
    }
    {
        const char *utf_ep = env->GetStringUTFChars(_verify_authority, nullptr);

        verify_authority_str = utf_ep;
        env->ReleaseStringUTFChars(_endpoint, utf_ep);
    }
    {
        const char *utf_ep = env->GetStringUTFChars(_client_cert, nullptr);

        client_cert_str = utf_ep;
        env->ReleaseStringUTFChars(_endpoint, utf_ep);
    }
    {
        const char *utf_ep = env->GetStringUTFChars(_client_key, nullptr);

        client_key_str = utf_ep;
        env->ReleaseStringUTFChars(_endpoint, utf_ep);
    }

    //g_ctx.echoActivityClz = env->GetObjectClass(_this);
    jclass clz = env->GetObjectClass(_this);
    g_ctx.echoActivityClz = (jclass)env->NewGlobalRef(clz);
    g_ctx.echoActivityObj = env->NewGlobalRef(_this);


    LOGI("native start %s secure %d auto_pilot %d", g_ctx.endpoint.c_str(), (int)(_secure == JNI_TRUE), (int)(_auto_pilot == JNI_TRUE));

    return JNI_TRUE;
    g_ctx.engine_ptr = bubbles::client::Engine::create(g_ctx.svc, g_ctx.ipcsvc, bubbles::client::EngineConfiguration{});

    ErrorConditionT			err;
    auto thr_enter = []() {

        JavaVM *javaVM = g_ctx.vm;
        JNIEnv *env = nullptr;
        jint res = javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
        if (res != JNI_OK) {
            res = javaVM->AttachCurrentThread(&env, NULL);
            if (JNI_OK != res) {
                LOGE("Failed to AttachCurrentThread, ErrorCode = %d", res);
                return false;
            }else{
                LOGI("Success AttachCurrentThread");
            }
        }

        local_java_env = env;
        //local_java_echo_receive_method_id = env->GetMethodID(g_ctx.echoActivityClz,
        //                                                     "onReceiveMessage", "(Ljava/lang/String;I)V");
        return true;
    };

    auto thr_exit = []() {
        g_ctx.vm->DetachCurrentThread();
    };

    err = g_ctx.sch.start(thr_enter, thr_exit, 1);

    if(err){
        //cout<<"Error starting aio scheduler: "<<err.message()<<endl;
        LOGE("native: Error starting scheduler: %s",err.message().c_str());
        return JNI_FALSE;
    }

    err = g_ctx.aio_sch.start(1);

    if(err){
        //cout<<"Error starting aio scheduler: "<<err.message()<<endl;
        LOGE("native: Error starting aio scheduler: %s",err.message().c_str());
        return JNI_FALSE;
    }

    err = g_ctx.resolver.start(1);

    if(err){
        //cout<<"Error starting aio resolver: "<<err.message()<<endl;
        LOGE("native: Error starting aio resolver: %s",err.message().c_str());
        return JNI_FALSE;
    }

    {
        auto 						proto = frame::mpipc::serialization_v1::Protocol::create(serialization::binary::Limits(256, 128, 0));//small limits by default
        frame::mpipc::Configuration	cfg(g_ctx.aio_sch, proto);

        bubbles::ProtoSpecT::setup<bubbles::client::MessageSetup>(*proto, 0, std::ref(*g_ctx.engine_ptr));

        cfg.client.name_resolve_fnc = frame::mpipc::InternetResolverF(g_ctx.resolver, "4444");

        cfg.client.connection_start_state = frame::mpipc::ConnectionState::Passive;

        {
            auto connection_stop_lambda = [](frame::mpipc::ConnectionContext &_ctx){
                g_ctx.engine_ptr->onConnectionStop(_ctx);
            };
            auto connection_start_lambda = [](frame::mpipc::ConnectionContext &_ctx){
                g_ctx.engine_ptr->onConnectionStart(_ctx);
            };
            cfg.connection_stop_fnc = connection_stop_lambda;
            cfg.client.connection_start_fnc = connection_start_lambda;
        }

        if(_secure == JNI_TRUE){
            //configure OpenSSL:
            idbg("Configure SSL ---------------------------------------");
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

        err = g_ctx.ipcsvc.reconfigure(std::move(cfg));

        if(err){
            cout<<"Error starting ipcservice: "<<err.message()<<endl;
            return 1;
        }
    }

    err = g_ctx.engine_ptr->start(g_ctx.sch, g_ctx.endpoint, g_ctx.room_name, _auto_pilot == JNI_TRUE);

    if(err){
        cout<<"Error starting engine: "<<err.message()<<endl;
        return 1;
    }
    g_ctx.started = true;
    return JNI_TRUE;
}

extern "C"
jboolean Java_com_example_vapa_bubbles_BubblesActivity_nativePause(){
    //g_ctx.ipcsvc.stop();
    LOGI("native: mpipc service stoped");
}

extern "C"
jboolean Java_com_example_vapa_bubbles_BubblesActivity_nativeResume(){
    //g_ctx.ipcsvc.start();
    LOGI("native: mpipc service started");
}