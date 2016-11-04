#ifndef BUBBLES_CLIENT_ENGINE_HPP
#define BUBBLES_CLIENT_ENGINE_HPP

#include "protocol/bubbles_messages.hpp"
#include "solid/frame/object.hpp"
#include "solid/frame/scheduler.hpp"
#include "solid/frame/reactor.hpp"
#include "solid/frame/service.hpp"
#include "solid/utility/dynamicpointer.hpp"

namespace solid{namespace frame{
namespace mpipc{
	class Service;
}
class ReactorContext;
}}

namespace bubbles{
namespace client{

using SchedulerT = solid::frame::Scheduler<solid::frame::Reactor>;

struct EngineConfiguration{
	EngineConfiguration(): max_event_queue_size(1024 * 8){}
	size_t		max_event_queue_size;
};

class Engine: public solid::Dynamic<Engine, solid::frame::Object>{
public:
	using PointerT = solid::DynamicPointer<Engine>;
	
	static PointerT create(
		solid::frame::ServiceT &_rsvc, solid::frame::mpipc::Service &_rmpipc,
		const EngineConfiguration &_cfg
	){
		return PointerT(new Engine(_rsvc, _rmpipc, _cfg));
	}
	
	solid::ErrorConditionT start(
		SchedulerT &_rsched,
		const std::string &_server_endpoint,
		const std::string &_room_name
	);
	
	void moveEvent(int _x, int _y);
	
private:
	Engine(
		solid::frame::ServiceT &_rsvc, solid::frame::mpipc::Service &_rmpipc,
		const EngineConfiguration &_cfg
	);
	~Engine();
public:
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
	void onEvent(solid::frame::ReactorContext &_rctx, solid::Event &&_uevent) override;
	void doTrySendEvents(std::shared_ptr<EventsNotification> &&_rrecv_msg_ptr = std::shared_ptr<EventsNotification>{});
private:
	struct Data;
	Data	&d;
};

}//namespace client
}//namespace bubbles

#endif
