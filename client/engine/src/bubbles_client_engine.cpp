#include "client/engine/bubbles_client_engine.hpp"
#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"

#include "solid/system/debug.hpp"
#include "solid/system/cassert.hpp"
#include "solid/utility/any.hpp"


using namespace solid;
using namespace std;

namespace bubbles{
namespace client{

struct Engine::Data{
	Data(solid::frame::mpipc::Service &_rmpipc):rmpipc(_rmpipc){}
	
	frame::mpipc::Service &rmpipc;
	
};

Engine::Engine(solid::frame::mpipc::Service &_rmpipc):d(*(new Data(_rmpipc))){}

Engine::~Engine(){
	delete &d;
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
	std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
	std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
	std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
}
	

}//namespace client
}//namespace bubbles
