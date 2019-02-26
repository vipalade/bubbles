#include "solid/system/log.hpp"
#include "solid/frame/manager.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/aio/aioresolver.hpp"

#include "solid/frame/mprpc/mprpcservice.hpp"
#include "solid/frame/mprpc/mprpcconfiguration.hpp"
#include "solid/frame/mprpc/mprpcsocketstub_openssl.hpp"
#include "solid/frame/mprpc/mprpccompression_snappy.hpp"

#include "protocol/bubbles_messages.hpp"

#include "bubbles_server_engine.hpp"

#include <signal.h>

#include "boost/program_options.hpp"

#include <iostream>

using namespace solid;
using namespace std;

using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;

//-----------------------------------------------------------------------------
//      Parameters
//-----------------------------------------------------------------------------
struct Parameters{
    Parameters():listener_port("0"), listener_addr("0.0.0.0"){}

    vector<string>          dbg_modules;
    string                  dbg_addr;
    string                  dbg_port;
    bool                    dbg_console;
    bool                    dbg_buffered;

    bool                    secure;
    bool                    compress;
    string                  listener_port;
    string                  listener_addr;
};

//-----------------------------------------------------------------------------

namespace bubbles{
namespace server{

struct MessageSetup {
    Engine &engine;

    MessageSetup(Engine &_engine):engine(_engine){}
    
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


}//namespace server
}//namespace bubbles

//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]);

//-----------------------------------------------------------------------------
//      main
//-----------------------------------------------------------------------------

int main(int argc, char *argv[]){
    Parameters params;

    if(parseArguments(params, argc, argv)) return 0;
    
    signal(SIGPIPE, SIG_IGN);
    
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

    {

        AioSchedulerT               scheduler;


        bubbles::server::Engine     engine(bubbles::server::EngineConfiguration{});
        frame::Manager              manager;
        frame::mprpc::ServiceT      ipcservice(manager);
        ErrorConditionT             err;

        scheduler.start(1);

        {
            auto                        proto = bubbles::ProtocolT::create();
            frame::mprpc::Configuration cfg(scheduler, proto);

            bubbles::protocol_setup(bubbles::server::MessageSetup(std::ref(engine)), *proto);
            
            cfg.limitString(256);
            cfg.limitContainer(256);
            cfg.limitStream(0);
            
            cfg.connection_inactivity_timeout_seconds = 60;

            cfg.server.listener_address_str = params.listener_addr;
            cfg.server.listener_address_str += ':';
            cfg.server.listener_address_str += params.listener_port;

            cfg.server.connection_start_state = frame::mprpc::ConnectionState::Active;
            //cfg.pool_max_message_queue_size
            {
                auto connection_stop_lambda = [&engine](frame::mprpc::ConnectionContext &_ctx){
                    engine.onConnectionStop(_ctx);
                };
                auto connection_start_lambda = [&engine](frame::mprpc::ConnectionContext &_ctx){
                    engine.onConnectionStart(_ctx);
                };
                cfg.connection_stop_fnc = std::move(connection_stop_lambda);
                cfg.server.connection_start_fnc = std::move(connection_start_lambda);
            }

            if(params.secure){
                frame::mprpc::openssl::setup_server(
                    cfg,
                    [](frame::aio::openssl::Context &_rctx) -> ErrorCodeT{
                        _rctx.loadVerifyFile("bubbles-ca-cert.pem"/*"/etc/pki/tls/certs/ca-bundle.crt"*/);
                        _rctx.loadCertificateFile("bubbles-server-cert.pem");
                        _rctx.loadPrivateKeyFile("bubbles-server-key.pem");
                        return ErrorCodeT();
                    },
                    frame::mprpc::openssl::NameCheckSecureStart{"bubbles-client"}//does nothing - OpenSSL does not check for hostname on SSL_accept
                );
            }

            if(params.compress){
                frame::mprpc::snappy::setup(cfg);
            }

            ipcservice.start(std::move(cfg));

            {
                std::ostringstream oss;
                oss<<ipcservice.configuration().server.listenerPort();
                cout<<"Server listens on port: "<<oss.str()<<endl;
            }
        }

        cout << "Press ENTER to stop:" << endl;
        cin.ignore();
        //cout<<"Max dropped message count: "<<engine.maxDroppedMessageCount()<<endl;
        engine.plotStatistics(cout);
    }
    return 0;
}

//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]){
    using namespace boost::program_options;
    try{
        options_description desc("Bubbles server");
        desc.add_options()
            ("help,h", "List program options")
            ("debug-modules,M", value<vector<string>>(&_par.dbg_modules),"Debug logging modules")
            ("debug-address,A", value<string>(&_par.dbg_addr), "Debug server address (e.g. on linux use: nc -l 9999)")
            ("debug-port,P", value<string>(&_par.dbg_port)->default_value("9999"), "Debug server port (e.g. on linux use: nc -l 9999)")
            ("debug-console,C", value<bool>(&_par.dbg_console)->implicit_value(true)->default_value(false), "Debug console")
            ("debug-unbuffered,S", value<bool>(&_par.dbg_buffered)->implicit_value(false)->default_value(true), "Debug unbuffered")

            ("listen-port,p", value<std::string>(&_par.listener_port)->default_value("4444"), "IPC Listen port")
            ("listen-addr,a", value<std::string>(&_par.listener_addr)->default_value("0.0.0.0"), "IPC Listen address")
            ("secure,s", value<bool>(&_par.secure)->implicit_value(true)->default_value(true), "Use SSL to secure communication")
            ("compress", value<bool>(&_par.compress)->implicit_value(true)->default_value(true), "Use Snappy to compress communication")
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


