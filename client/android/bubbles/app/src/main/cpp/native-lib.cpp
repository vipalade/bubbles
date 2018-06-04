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
#include "solid/frame/mpipc/mpipcprotocol_serialization_v2.hpp"
#include "solid/frame/mpipc/mpipcsocketstub_openssl.hpp"
#include "solid/frame/mpipc/mpipccompression_snappy.hpp"

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

    using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;
    using SchedulerT = frame::Scheduler<frame::Reactor>;
    using SecureContextT = frame::aio::openssl::Context;
    using BubblesEnginePointerT = bubbles::client::Engine::PointerT;
    using PlotIteratorT = bubbles::client::PlotIterator;

    struct Context {
        Context(

        ):  vm(nullptr), done(false), started(false), bubblesActivityClz(nullptr), bubblesActivityObj(nullptr),
            m{}, ipcsvc(m), svc(m), resolver(fwp){}

        JavaVM                  *vm;
        bool                    done;
        bool                    started;
        jclass                  bubblesActivityClz;
        jobject                 bubblesActivityObj;

        std::string             endpoint;
        std::string             room_name;

        AioSchedulerT           aio_sch;
        SchedulerT              sch;


        frame::Manager          m;
        frame::mpipc::ServiceT  ipcsvc;
        frame::ServiceT         svc;
        FunctionWorkPool        fwp;
        frame::aio::Resolver    resolver;
        BubblesEnginePointerT   engine_ptr;
        PlotIteratorT           plotit;
    }g_ctx;

    thread_local JNIEnv                  *local_java_env = nullptr;
    thread_local jmethodID               local_java_exit_method_id;
    thread_local jmethodID               local_java_auto_method_id;
    thread_local jmethodID               local_java_gui_update_method_id;


    void exit_callback();
    void gui_update_callback();
    void auto_move_callback();

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
                    frame::mpipc::ConnectionContext &_rctx,
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
                    frame::mpipc::ConnectionContext &_rctx,
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
                    frame::mpipc::ConnectionContext &_rctx,
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
                    frame::mpipc::ConnectionContext &_rctx,
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
                    frame::mpipc::ConnectionContext &_rctx,
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
        jobject _this, jstring _endpoint, jstring _room, jboolean _secure, jboolean _compressed, jboolean _auto_pilot,
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
    g_ctx.bubblesActivityClz = (jclass)env->NewGlobalRef(clz);
    g_ctx.bubblesActivityObj = env->NewGlobalRef(_this);


    LOGI("native start %s secure %d auto_pilot %d", g_ctx.endpoint.c_str(), (int)(_secure == JNI_TRUE), (int)(_auto_pilot == JNI_TRUE));

    g_ctx.engine_ptr = bubbles::client::Engine::create(g_ctx.svc, g_ctx.ipcsvc, bubbles::client::EngineConfiguration{});

    ErrorConditionT         err;
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
        local_java_exit_method_id = env->GetMethodID(g_ctx.bubblesActivityClz, "onNativeRequestExit", "()V");
        local_java_auto_method_id = env->GetMethodID(g_ctx.bubblesActivityClz, "onNativeRequestAutoUpdate", "(II)V");
        local_java_gui_update_method_id = env->GetMethodID(g_ctx.bubblesActivityClz, "onNativeRequestGuiUpdate", "()V");
        //local_java_echo_receive_method_id = env->GetMethodID(g_ctx.echoActivityClz,
        //                                                     "onReceiveMessage", "(Ljava/lang/String;I)V");
        return true;
    };

    auto thr_exit = []() {
        g_ctx.vm->DetachCurrentThread();
    };

    err = g_ctx.sch.start(std::move(thr_enter), std::move(thr_exit), 1);

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
    
    fwp.start(WorkPoolConfiguration());

    {
        auto                        proto = bubbles::ProtocolT::create();//small limits by default
        frame::mpipc::Configuration cfg(g_ctx.aio_sch, proto);

        bubbles::protocol_setup(bubbles::client::MessageSetup(std::ref(*g_ctx.engine_ptr)), *proto);

        cfg.client.name_resolve_fnc = frame::mpipc::InternetResolverF(g_ctx.resolver, "4444");

        cfg.client.connection_start_state = frame::mpipc::ConnectionState::Passive;

        cfg.connection_keepalive_timeout_seconds = 30;

        {
            auto connection_stop_lambda = [](frame::mpipc::ConnectionContext &_ctx){
                LOGI("connection stop");
                g_ctx.engine_ptr->onConnectionStop(_ctx);
            };
            auto connection_start_lambda = [](frame::mpipc::ConnectionContext &_ctx){
                LOGI("connection start");
                g_ctx.engine_ptr->onConnectionStart(_ctx);
            };
            cfg.connection_stop_fnc = std::move(connection_stop_lambda);
            cfg.client.connection_start_fnc = std::move(connection_start_lambda);
        }

        if(_secure == JNI_TRUE){
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

        if(_compressed == JNI_TRUE){
            //configure Snappy compression:
            frame::mpipc::snappy::setup(cfg);
        }

        err = g_ctx.ipcsvc.reconfigure(std::move(cfg));

        if(err){
            LOGE("Error starting ipcservice: %s",err.message().c_str());
            return JNI_FALSE;
        }
    }

    {
        //set the callbacks

        g_ctx.engine_ptr->setExitFunction(exit_callback);
        g_ctx.engine_ptr->setAutoUpdateFunction(auto_move_callback);
        g_ctx.engine_ptr->setGuiUpdateFunction(gui_update_callback);

    }

    err = g_ctx.engine_ptr->start(g_ctx.sch, g_ctx.endpoint, g_ctx.room_name, _auto_pilot == JNI_TRUE);

    if(err){
        LOGE("Error starting engine: %s",err.message().c_str());
        return JNI_FALSE;
    }
    g_ctx.started = true;
    return JNI_TRUE;
}

extern "C"
jboolean Java_com_example_vapa_bubbles_BubblesActivity_nativePause(
        JNIEnv *env,
        jobject _this){

    LOGI("native: engine stoped");
    g_ctx.engine_ptr->pause();
    return JNI_TRUE;
}

extern "C"
jboolean Java_com_example_vapa_bubbles_BubblesActivity_nativeResume(
        JNIEnv *env,
        jobject _this){
    LOGI("native: engine started");
    g_ctx.engine_ptr->resume();
    return JNI_TRUE;
}
extern "C"
jboolean Java_com_example_vapa_bubbles_BubblesActivity_nativeMove(
        JNIEnv *env,
        jobject _this, jint _x, jint _y){

    //LOGI("move %d,%d", (int)_x, (int)_y);
    g_ctx.engine_ptr->moveEvent(_x, _y);
    return JNI_TRUE;
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativeScaleX(
        JNIEnv *env,
        jobject _this, jint _x, jint _w){

    return g_ctx.engine_ptr->scaleX(_x, _w);
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativeScaleY(
        JNIEnv *env,
        jobject _this, jint _y, jint _h){

    return g_ctx.engine_ptr->scaleY(_y, _h);
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativeReverseScaleX(
        JNIEnv *env,
        jobject _this, jint _x, jint _w){

    return g_ctx.engine_ptr->reverseScaleX(_x, _w);
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativeReverseScaleY(
        JNIEnv *env,
        jobject _this, jint _y, jint _h){

    return g_ctx.engine_ptr->reverseScaleY(_y, _h);
}

namespace{
    inline jint java_color(uint32_t _c){
        int r = _c & 0xff;
        int g = (_c >> 8) & 0xff;
        int b = (_c >> 16) & 0xff;
        int v = (0xff000000 | (b << 0) | (g << 8) | (r << 16));
        return v;
    }
}

extern "C"
void Java_com_example_vapa_bubbles_BubblesActivity_nativePlotStart(JNIEnv *env, jobject _this){
    g_ctx.plotit = g_ctx.engine_ptr->plot();
}

extern "C"
jboolean Java_com_example_vapa_bubbles_BubblesActivity_nativePlotEnd(JNIEnv *env, jobject _this){
    return g_ctx.plotit.end() ? JNI_TRUE : JNI_FALSE;
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativePlotX(JNIEnv *env, jobject _this){
    return g_ctx.plotit.x();
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativePlotY(JNIEnv *env, jobject _this){
    return g_ctx.plotit.y();
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativePlotColor(JNIEnv *env, jobject _this){
    return java_color(g_ctx.plotit.rgbColor());
}

extern "C"
jint Java_com_example_vapa_bubbles_BubblesActivity_nativePlotMyColor(JNIEnv *env, jobject _this){
    return java_color(g_ctx.plotit.myRgbColor());
}

extern "C"
void Java_com_example_vapa_bubbles_BubblesActivity_nativePlotDone(JNIEnv *env, jobject _this){
    return g_ctx.plotit.clear();
}

extern "C"
void Java_com_example_vapa_bubbles_BubblesActivity_nativePlotNext(JNIEnv *env, jobject _this){
    ++g_ctx.plotit;
}


namespace {
    void exit_callback(){
        LOGI("exit --------");
        local_java_env->CallVoidMethod(g_ctx.bubblesActivityObj, local_java_exit_method_id);
    }
    void gui_update_callback(){
        //LOGI("gui update");
        local_java_env->CallVoidMethod(g_ctx.bubblesActivityObj, local_java_gui_update_method_id);
    }
    void auto_move_callback(){
        int x,y;
        g_ctx.engine_ptr->getAutoPosition(x, y);
        //LOGI("auto update %d.%d", x, y);
        local_java_env->CallVoidMethod(g_ctx.bubblesActivityObj, local_java_auto_method_id, x, y);
    }
}
