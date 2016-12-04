#include <QtGui>
#include <QApplication>
#include "bubbles_client_widget.hpp"


#include "solid/system/debug.hpp"
#include "solid/frame/manager.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/aio/aioresolver.hpp"

#include "solid/frame/reactor.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"
#include "solid/frame/mpipc/mpipcsocketstub_openssl.hpp"

#include "client/engine/bubbles_client_engine.hpp"

#include "protocol/bubbles_messages.hpp"

#include "boost/program_options.hpp"

#include <iostream>

using namespace solid;
using namespace std;

using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;
using SchedulerT = frame::Scheduler<frame::Reactor>;

//-----------------------------------------------------------------------------
//		Parameters
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
	
	string					dbg_levels;
	string					dbg_modules;
	string					dbg_addr;
	string					dbg_port;
	bool					dbg_console;
	bool					dbg_buffered;
	
	bool					secure;
	bool					auto_pilot;
	
	string					connect_endpoint;
	string					connect_addr;
	string					connect_port;
	
	string					room_name;
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


	QApplication 						app(argc, argv);
	
	AioSchedulerT						aioscheduler;
	SchedulerT							scheduler;
	
	
	frame::Manager						manager;
	frame::ServiceT						service{manager};
	
	frame::mpipc::ServiceT				ipcservice{manager};
	
	frame::aio::Resolver				resolver;
	
	ErrorConditionT						err;
	bubbles::client::Engine::PointerT	engine_ptr{bubbles::client::Engine::create(service, ipcservice, bubbles::client::EngineConfiguration{})};
	
	bubbles::client::Widget				widget{engine_ptr};
	
	err = aioscheduler.start(1);
	
	if(err){
		cout<<"Error starting aio scheduler: "<<err.message()<<endl;
		return 1;
	}
	
	err = resolver.start(1);
	
	if(err){
		cout<<"Error starting aio resolver: "<<err.message()<<endl;
		return 1;
	}
	
	err = scheduler.start(1);
	
	if(err){
		cout<<"Error starting scheduler: "<<err.message()<<endl;
		return 1;
	}
	
	{
		auto 						proto = frame::mpipc::serialization_v1::Protocol::create(serialization::binary::Limits(256, 128, 0));//small limits by default
		frame::mpipc::Configuration	cfg(aioscheduler, proto);
		
		bubbles::ProtoSpecT::setup<bubbles::client::MessageSetup>(*proto, 0, std::ref(*engine_ptr));
		
		cfg.client.name_resolve_fnc = frame::mpipc::InternetResolverF(resolver, p.connect_port.c_str());
		
		cfg.client.connection_start_state = frame::mpipc::ConnectionState::Passive;
		
		{
			auto connection_stop_lambda = [engine_ptr](frame::mpipc::ConnectionContext &_ctx){
				engine_ptr->onConnectionStop(_ctx);
			};
			auto connection_start_lambda = [engine_ptr](frame::mpipc::ConnectionContext &_ctx){
				engine_ptr->onConnectionStart(_ctx);
			};
			cfg.connection_stop_fnc = connection_stop_lambda;
			cfg.client.connection_start_fnc = connection_start_lambda;
		}
		
		if(p.secure){
			//configure OpenSSL:
			idbg("Configure SSL ---------------------------------------");
			frame::mpipc::openssl::setup_client(
				cfg,
				[](frame::aio::openssl::Context &_rctx) -> ErrorCodeT{
					_rctx.loadVerifyFile("bubbles-ca-cert.pem");
					_rctx.loadCertificateFile("bubbles-client-cert.pem");
					_rctx.loadPrivateKeyFile("bubbles-client-key.pem");
					return ErrorCodeT();
				},
				frame::mpipc::openssl::NameCheckSecureStart{"bubbles-server"}
			);
		}
		
		err = ipcservice.reconfigure(std::move(cfg));
		
		if(err){
			cout<<"Error starting ipcservice: "<<err.message()<<endl;
			return 1;
		}
	}
	
	err = engine_ptr->start(scheduler, p.connect_endpoint, p.room_name, p.auto_pilot);
	
	if(err){
		cout<<"Error starting engine: "<<err.message()<<endl;
		return 1;
	}
	
	widget.start();
	
	int rv = app.exec();
	
	
	
	return rv;
}

//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]){
	using namespace boost::program_options;
	try{
		options_description desc("Bubbles client");
		desc.add_options()
			("help,h", "List program options")
			("debug-levels,L", value<string>(&_par.dbg_levels)->default_value("view"),"Debug logging levels")
			("debug-modules,M", value<string>(&_par.dbg_modules),"Debug logging modules")
			("debug-address,A", value<string>(&_par.dbg_addr), "Debug server address (e.g. on linux use: nc -l 9999)")
			("debug-port,P", value<string>(&_par.dbg_port)->default_value("9999"), "Debug server port (e.g. on linux use: nc -l 9999)")
			("debug-console,C", value<bool>(&_par.dbg_console)->implicit_value(true)->default_value(false), "Debug console")
			("debug-unbuffered,S", value<bool>(&_par.dbg_buffered)->implicit_value(false)->default_value(true), "Debug unbuffered")
			
			("connect,c", value<std::string>(&_par.connect_endpoint)->default_value("viphost.asuscomm.com:36444"), "Server endpoint: address:port")
			("room,r", value<std::string>(&_par.room_name)->default_value("test"), "Connect room")
			("secure,s", value<bool>(&_par.secure)->implicit_value(true)->default_value(true), "Use SSL to secure communication")
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

