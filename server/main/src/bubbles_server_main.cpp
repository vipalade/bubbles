#include "solid/system/debug.hpp"
#include "solid/frame/manager.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/aio/aioresolver.hpp"

#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"
#include "solid/frame/mpipc/mpipcsocketstub_openssl.hpp"

#include "protocol/bubbles_messages.hpp"

#include "bubbles_server_engine.hpp"


#include "boost/program_options.hpp"

#include <iostream>

using namespace solid;
using namespace std;

using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;

//-----------------------------------------------------------------------------
//		Parameters
//-----------------------------------------------------------------------------
struct Parameters{
	Parameters():listener_port("0"), listener_addr("0.0.0.0"){}
	
	string					dbg_levels;
	string					dbg_modules;
	string					dbg_addr;
	string					dbg_port;
	bool					dbg_console;
	bool					dbg_buffered;
	
	bool					secure;
	string					listener_port;
	string					listener_addr;
};

//-----------------------------------------------------------------------------

namespace bubbles{
namespace server{

template <typename T>
struct MessageSetup;

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

}//namespace server
}//namespace bubbles

//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]);

//-----------------------------------------------------------------------------
//		main
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
		
		AioSchedulerT				scheduler;
		
		
		bubbles::server::Engine		engine(bubbles::server::EngineConfiguration{});
		frame::Manager				manager;
		frame::mpipc::ServiceT		ipcservice(manager);
		ErrorConditionT				err;
		
		err = scheduler.start(1);
		
		if(err){
			cout<<"Error starting aio scheduler: "<<err.message()<<endl;
			return 1;
		}
		
		{
			auto						proto = frame::mpipc::serialization_v1::Protocol::create(serialization::binary::Limits(256, 128, 0));//small limits by default
			frame::mpipc::Configuration	cfg(scheduler, proto);
			
			bubbles::ProtoSpecT::setup<bubbles::server::MessageSetup>(*proto, 0, std::ref(engine));
			
			cfg.server.listener_address_str = p.listener_addr;
			cfg.server.listener_address_str += ':';
			cfg.server.listener_address_str += p.listener_port;
			
			cfg.server.connection_start_state = frame::mpipc::ConnectionState::Active;
			//cfg.pool_max_message_queue_size
			{
				auto connection_stop_lambda = [&engine](frame::mpipc::ConnectionContext &_ctx){
					engine.onConnectionStop(_ctx);
				};
				auto connection_start_lambda = [&engine](frame::mpipc::ConnectionContext &_ctx){
					engine.onConnectionStart(_ctx);
				};
				cfg.connection_stop_fnc = connection_stop_lambda;
				cfg.server.connection_start_fnc = connection_start_lambda;
			}
			
			if(p.secure){
				frame::mpipc::openssl::setup_server(
					cfg,
					[](frame::aio::openssl::Context &_rctx) -> ErrorCodeT{
						_rctx.loadVerifyFile("bubbles-ca-cert.pem"/*"/etc/pki/tls/certs/ca-bundle.crt"*/);
						_rctx.loadCertificateFile("bubbles-server-cert.pem");
						_rctx.loadPrivateKeyFile("bubbles-server-key.pem");
						return ErrorCodeT();
					},
					frame::mpipc::openssl::NameCheckSecureStart{"bubbles-client"}
				);
			}
			
			err = ipcservice.reconfigure(std::move(cfg));
			
			if(err){
				cout<<"Error starting ipcservice: "<<err.message()<<endl;
				manager.stop();
				return 1;
			}
			{
				std::ostringstream oss;
				oss<<ipcservice.configuration().server.listenerPort();
				cout<<"Server listens on port: "<<oss.str()<<endl;
			}
		}
		
		cout<<"Press any char and ENTER to stop: ";
		char c;
		cin>>c;
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
			("debug-levels,L", value<string>(&_par.dbg_levels)->default_value("view"),"Debug logging levels")
			("debug-modules,M", value<string>(&_par.dbg_modules),"Debug logging modules")
			("debug-address,A", value<string>(&_par.dbg_addr), "Debug server address (e.g. on linux use: nc -l 9999)")
			("debug-port,P", value<string>(&_par.dbg_port)->default_value("9999"), "Debug server port (e.g. on linux use: nc -l 9999)")
			("debug-console,C", value<bool>(&_par.dbg_console)->implicit_value(true)->default_value(false), "Debug console")
			("debug-unbuffered,S", value<bool>(&_par.dbg_buffered)->implicit_value(false)->default_value(true), "Debug unbuffered")
			
			("listen-port,p", value<std::string>(&_par.listener_port)->default_value("4444"), "IPC Listen port")
			("listen-addr,a", value<std::string>(&_par.listener_addr)->default_value("0.0.0.0"), "IPC Listen address")
			("secure,s", value<bool>(&_par.secure)->implicit_value(true)->default_value(true), "Use SSL to secure communication")
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


