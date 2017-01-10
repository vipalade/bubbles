#include "solid/system/debug.hpp"
#include "solid/frame/manager.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/aio/aioresolver.hpp"

#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"

#include "client/engine/bubbles_client_engine.hpp"
#include "protocol/bubbles_messages.hpp"

#include "boost/program_options.hpp"

#include <iostream>

using namespace solid;
using namespace std;

using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;

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

    string                  dbg_levels;
    string                  dbg_modules;
    string                  dbg_addr;
    string                  dbg_port;
    bool                    dbg_console;
    bool                    dbg_buffered;

    bool                    secure;

    string                  connect_endpoint;
    string                  connect_addr;
    string                  connect_port;
};

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

bool parseArguments(Parameters &_par, int argc, char *argv[]);

//-----------------------------------------------------------------------------
//      main
//-----------------------------------------------------------------------------

int main(int argc, char *argv[]){
    Parameters p;

    if(parseArguments(p, argc, argv)) return 0;

#ifdef SOLID_HAS_DEBUG
    {
    string dbgout;
    Debug::the().levelMask(p.dbg_levels.c_str());
    Debug::the().moduleMask(p.dbg_modules.c_str());
    if(p.dbg_addr.size() && p.dbg_port.size()){
        Debug::the().initSocket(
            p.dbg_addr.c_str(),
            p.dbg_port.c_str(),
            p.dbg_buffered,
            &dbgout
        );
    }else if(p.dbg_console){
        Debug::the().initStdErr(
            p.dbg_buffered,
            &dbgout
        );
    }else{
        Debug::the().initFile(
            *argv[0] == '.' ? argv[0] + 2 : argv[0],
            p.dbg_buffered,
            3,
            1024 * 1024 * 64,
            &dbgout
        );
    }
    cout<<"Debug output: "<<dbgout<<endl;
    dbgout.clear();
    Debug::the().moduleNames(dbgout);
    cout<<"Debug modules: "<<dbgout<<endl;
    }
#endif
    {

        AioSchedulerT               scheduler;


        frame::Manager              manager;
        frame::mpipc::ServiceT      ipcservice(manager);

        frame::aio::Resolver        resolver;

        ErrorConditionT             err;
        bubbles::client::Engine     engine{ipcservice};

        err = scheduler.start(1);

        if(err){
            cout<<"Error starting aio scheduler: "<<err.message()<<endl;
            return 1;
        }

        err = resolver.start(1);

        if(err){
            cout<<"Error starting aio resolver: "<<err.message()<<endl;
            return 1;
        }

        {
            auto                        proto = frame::mpipc::serialization_v1::Protocol::create(serialization::binary::Limits(256, 128, 0));//small limits by default
            frame::mpipc::Configuration cfg(scheduler, proto);

            bubbles::ProtoSpecT::setup<bubbles::client::MessageSetup>(*proto, 0, std::ref(engine));

            cfg.client.name_resolve_fnc = frame::mpipc::InternetResolverF(resolver, p.connect_port.c_str());

            cfg.client.connection_start_state = frame::mpipc::ConnectionState::Passive;

            err = ipcservice.reconfigure(std::move(cfg));

            if(err){
                cout<<"Error starting ipcservice: "<<err.message()<<endl;
                return 1;
            }
        }


        while(true){
            string  line;
            getline(cin, line);

            if(line == "q" or line == "Q" or line == "quit"){
                break;
            }
            {
            }
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]){
    using namespace boost::program_options;
    try{
        options_description desc("SolidFrame ipc stress test");
        desc.add_options()
            ("help,h", "List program options")
            ("debug-levels,L", value<string>(&_par.dbg_levels)->default_value("view"),"Debug logging levels")
            ("debug-modules,M", value<string>(&_par.dbg_modules),"Debug logging modules")
            ("debug-address,A", value<string>(&_par.dbg_addr), "Debug server address (e.g. on linux use: nc -l 9999)")
            ("debug-port,P", value<string>(&_par.dbg_port)->default_value("9999"), "Debug server port (e.g. on linux use: nc -l 9999)")
            ("debug-console,C", value<bool>(&_par.dbg_console)->implicit_value(true)->default_value(false), "Debug console")
            ("debug-unbuffered,S", value<bool>(&_par.dbg_buffered)->implicit_value(false)->default_value(true), "Debug unbuffered")

            ("connect,c", value<std::string>(&_par.connect_endpoint)->default_value("localhost:2000"), "Server endpoint: address:port")
            ("secure,s", value<bool>(&_par.secure)->implicit_value(true)->default_value(false), "Use SSL to secure communication")
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



