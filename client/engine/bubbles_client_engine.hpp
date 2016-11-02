#ifndef BUBBLES_CLIENT_ENGINE_HPP
#define BUBBLES_CLIENT_ENGINE_HPP

#include "protocol/bubbles_messages.hpp"

namespace solid{namespace frame{namespace mpipc{
	class Service;
}}}

namespace bubbles{
namespace client{

class Engine{
public:
	Engine(solid::frame::mpipc::Service &_rmpipc);
	~Engine();
	
	void onMessage(
		solid::frame::mpipc::ConnectionContext &_rctx,
		std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
		std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
		solid::ErrorConditionT const &_rerror
	);
	
	void onMessage(
		solid::frame::mpipc::ConnectionContext &_rctx,
		std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
		std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
		solid::ErrorConditionT const &_rerror
	);
	
	void onMessage(
		solid::frame::mpipc::ConnectionContext &_rctx,
		std::shared_ptr<EventsNotificationRequest> &_rsent_msg_ptr,
		std::shared_ptr<EventsNotificationResponse> &_rrecv_msg_ptr,
		solid::ErrorConditionT const &_rerror
	);
	
private:
	struct Data;
	Data	&d;
};

}//namespace client
}//namespace bubbles

#endif
