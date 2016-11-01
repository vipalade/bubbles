#include "solid/frame/manager.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/service.hpp"

#include "solid/frame/aio/aioresolver.hpp"

#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"

#include "protocol/bubbles_messages.hpp"

#include "bubbles_server_engine.hpp"

#include <iostream>

using namespace solid;
using namespace std;

using AioSchedulerT = frame::Scheduler<frame::aio::Reactor>;

//-----------------------------------------------------------------------------
//		Parameters
//-----------------------------------------------------------------------------
struct Parameters{
	Parameters():listener_port("0"), listener_addr("0.0.0.0"){}
	
	string			listener_port;
	string			listener_addr;
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
	
	if(!parseArguments(p, argc, argv)) return 0;
	
	{
		
		AioSchedulerT				scheduler;
		
		
		frame::Manager				manager;
		frame::mpipc::ServiceT		ipcservice(manager);
		ErrorConditionT				err;
		bubbles::server::Engine		engine;
		
		err = scheduler.start(1);
		
		if(err){
			cout<<"Error starting aio scheduler: "<<err.message()<<endl;
			return 1;
		}
		
		{
			auto						proto = frame::mpipc::serialization_v1::Protocol::create();
			frame::mpipc::Configuration	cfg(scheduler, proto);
			
			bubbles::ProtoSpecT::setup<bubbles::server::MessageSetup>(*proto, 0, std::ref(engine));
			
			cfg.server.listener_address_str = p.listener_addr;
			cfg.server.listener_address_str += ':';
			cfg.server.listener_address_str += p.listener_port;
			
			cfg.server.connection_start_state = frame::mpipc::ConnectionState::Active;
			
			err = ipcservice.reconfigure(std::move(cfg));
			
			if(err){
				cout<<"Error starting ipcservice: "<<err.message()<<endl;
				manager.stop();
				return 1;
			}
			{
				std::ostringstream oss;
				oss<<ipcservice.configuration().server.listenerPort();
				cout<<"server listens on port: "<<oss.str()<<endl;
			}
		}
		
		cout<<"Press any char and ENTER to stop: ";
		char c;
		cin>>c;
	}
	return 0;
}

//-----------------------------------------------------------------------------

bool parseArguments(Parameters &_par, int argc, char *argv[]){
	if(argc == 2){
		size_t			pos;
		
		_par.listener_addr = argv[1];
		
		pos = _par.listener_addr.rfind(':');
		
		if(pos != string::npos){
			_par.listener_addr[pos] = '\0';
			
			_par.listener_port.assign(_par.listener_addr.c_str() + pos + 1);
			
			_par.listener_addr.resize(pos);
		}
	}
	return true;
}


