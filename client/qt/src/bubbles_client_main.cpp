#include "bubbles_client_widget.hpp"


#include "solid/system/log.hpp"
#include "solid/frame/manager.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/aio/aioresolver.hpp"

#include "solid/frame/reactor.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"
#include "solid/frame/mpipc/mpipcsocketstub_openssl.hpp"
#include "solid/frame/mpipc/mpipccompression_snappy.hpp"


#include "client/engine/bubbles_client_engine.hpp"

#include "protocol/bubbles_messages.hpp"

#include "boost/program_options.hpp"

#include <QtGui>
#include <QApplication>

#include <signal.h>

#include <iostream>
#include <fstream>

using namespace solid;
using namespace std;

using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;
using SchedulerT = frame::Scheduler<frame::Reactor>;

//-----------------------------------------------------------------------------
//      Parameters
//-----------------------------------------------------------------------------
struct Parameters{
    Parameters(){}

    void prepare(){
        size_t pos = connect_endpoint.rfind(':');
        if(pos == string::npos){
            connect_addr = connect_endpoint;
        }else{
            connect_addr = connect_endpoint.substr(0, pos);
            connect_port = connect_endpoint.substr(pos + 1);
        }
    }

    vector<string>          dbg_modules;
    string                  dbg_addr;
    string                  dbg_port;
    bool                    dbg_console;
    bool                    dbg_buffered;

    bool                    secure;
    bool                    compress;
    bool                    auto_pilot;

    string                  connect_endpoint;
    string                  connect_addr;
    string                  connect_port;

    string                  room_name;
};

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

namespace{
string load_file(const char *_fname){
    std::ifstream t(_fname);
    return std::string((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
}
}
//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]);

//-----------------------------------------------------------------------------
//      main
//-----------------------------------------------------------------------------

int main(int argc, char *argv[]){
    Parameters params;
    
    if(parseArguments(params, argc, argv)) return 0;
#if !defined(SOLID_ON_WINDOWS)
    signal(SIGPIPE, SIG_IGN);
#endif
    
    if (params.dbg_addr.size() && params.dbg_port.size()) {
        solid::log_start(
            params.dbg_addr.c_str(),
            params.dbg_port.c_str(),
            params.dbg_modules,
            params.dbg_buffered);

    } else if (params.dbg_console) {
        solid::log_start(std::cerr, params.dbg_modules);
    } else {
        solid::log_start(
            *argv[0] == '.' ? argv[0] + 2 : argv[0],
            params.dbg_modules,
            params.dbg_buffered,
            3,
            1024 * 1024 * 64);
    }


    QApplication                        app(argc, argv);

    AioSchedulerT                       aioscheduler;
    SchedulerT                          scheduler;


    frame::Manager                      manager;
    frame::ServiceT                     service{manager};

    frame::mpipc::ServiceT              ipcservice{manager};
    
    FunctionWorkPool                    fwp{WorkPoolConfiguration()};
    frame::aio::Resolver                resolver(fwp);

    ErrorConditionT                     err;
    bubbles::client::Engine::PointerT   engine_ptr{bubbles::client::Engine::create(service, ipcservice, bubbles::client::EngineConfiguration{})};

    bubbles::client::Widget             widget{engine_ptr};

    err = aioscheduler.start(1);

    if(err){
        cout<<"Error starting aio scheduler: "<<err.message()<<endl;
        return 1;
    }
    
    err = scheduler.start(1);

    if(err){
        cout<<"Error starting scheduler: "<<err.message()<<endl;
        return 1;
    }

    {
        auto                        proto = bubbles::ProtocolT::create();//small limits by default
        frame::mpipc::Configuration cfg(aioscheduler, proto);

        bubbles::protocol_setup(bubbles::client::MessageSetup(std::ref(*engine_ptr)), *proto);
        
        cfg.limitString(256);
        cfg.limitContainer(256);
        cfg.limitStream(0);
        
        cfg.client.name_resolve_fnc = frame::mpipc::InternetResolverF(resolver, params.connect_port.c_str());

        cfg.client.connection_start_state = frame::mpipc::ConnectionState::Passive;
        
        cfg.connection_keepalive_timeout_seconds = 30;

        {
            auto connection_stop_lambda = [engine_ptr](frame::mpipc::ConnectionContext &_ctx){
                engine_ptr->onConnectionStop(_ctx);
            };
            auto connection_start_lambda = [engine_ptr](frame::mpipc::ConnectionContext &_ctx){
                engine_ptr->onConnectionStart(_ctx);
            };
            cfg.connection_stop_fnc = std::move(connection_stop_lambda);
            cfg.client.connection_start_fnc = std::move(connection_start_lambda);
        }

        if(params.secure){
            //configure OpenSSL:
            solid_log(generic_logger, Info, "Configure SSL ---------------------------------------");
            frame::mpipc::openssl::setup_client(
                cfg,
                [](frame::aio::openssl::Context &_rctx) -> ErrorCodeT{
                    if(true){
                        const string verify_authority_str = load_file("bubbles-ca-cert.pem");
                        const string client_cert_str = load_file("bubbles-client-cert.pem");
                        const string client_key_str = load_file("bubbles-client-key.pem");

                        _rctx.addVerifyAuthority(verify_authority_str);
                        _rctx.loadCertificate(client_cert_str);
                        _rctx.loadPrivateKey(client_key_str);
                    }else{
                        _rctx.loadVerifyFile("bubbles-ca-cert.pem");
                        _rctx.loadCertificateFile("bubbles-client-cert.pem");
                        _rctx.loadPrivateKeyFile("bubbles-client-key.pem");
                    }
                    return ErrorCodeT();
                },
                frame::mpipc::openssl::NameCheckSecureStart{"bubbles-server"}
            );
        }

        if(params.compress){
            frame::mpipc::snappy::setup(cfg);
        }

        err = ipcservice.reconfigure(std::move(cfg));

        if(err){
            cout<<"Error starting ipcservice: "<<err.message()<<endl;
            return 1;
        }
    }

    err = engine_ptr->start(scheduler, params.connect_endpoint, params.room_name, params.auto_pilot);

    if(err){
        cout<<"Error starting engine: "<<err.message()<<endl;
        return 1;
    }

    widget.start();

    int rv = app.exec();
    
    engine_ptr->pause();

    return rv;
}

//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]){
    using namespace boost::program_options;
    try{
        options_description desc("Bubbles client");
        desc.add_options()
            ("help,h", "List program options")
            ("debug-modules,M", value<vector<string>>(&_par.dbg_modules),"Debug logging modules (e.g. \".*:EW\", \"\\*:VIEW\")")
            ("debug-address,A", value<string>(&_par.dbg_addr), "Debug server address (e.g. on linux use: nc -l 9999)")
            ("debug-port,P", value<string>(&_par.dbg_port)->default_value("9999"), "Debug server port (e.g. on linux use: nc -l 9999)")
            ("debug-console,C", value<bool>(&_par.dbg_console)->implicit_value(true)->default_value(false), "Debug console")
            ("debug-unbuffered,S", value<bool>(&_par.dbg_buffered)->implicit_value(false)->default_value(true), "Debug unbuffered")

            ("connect,c", value<std::string>(&_par.connect_endpoint)->default_value("viphost.asuscomm.com:36444"), "Server endpoint: address:port")
            ("room,r", value<std::string>(&_par.room_name)->default_value("test"), "Connect room")
            ("secure,s", value<bool>(&_par.secure)->implicit_value(true)->default_value(true), "Use SSL to secure communication")
            ("compress", value<bool>(&_par.compress)->implicit_value(true)->default_value(true), "Use Snappy to compress communication")
            ("auto,a", value<bool>(&_par.auto_pilot)->implicit_value(true)->default_value(true), "Auto randomly move the bubble")
        ;
        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);
        notify(vm);
        if (vm.count("help")) {
            cout << desc << "\n";
            return true;
        }
        return false;
    }catch(exception& e){
        cout << e.what() << "\n";
        return true;
    }
}

//-----------------------------------------------------------------------------

