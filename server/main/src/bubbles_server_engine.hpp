#ifndef BUBBLES_SERVER_ENGINE_HPP
#define BUBBLES_SERVER_ENGINE_HPP

#include "protocol/bubbles_messages.hpp"
#include "solid/system/error.hpp"

namespace solid{namespace frame{namespace mpipc{
struct ConnectionContext;
}}}

namespace bubbles{
namespace server{

struct ConnectionData;

class Engine{
public:
	Engine(const Engine&) = delete;
	Engine(Engine&&) = delete;
	Engine& operator=(const Engine&) = delete;
	Engine& operator=(Engine&&) = delete;
	
	Engine();
	
	
	void onConnectionStart(solid::frame::mpipc::ConnectionContext &_rctx);
	void onConnectionStop(solid::frame::mpipc::ConnectionContext &_rctx);
	
	void onMessage(
		solid::frame::mpipc::ConnectionContext &_rctx,
		std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
		std::shared_ptr<RegisterRequest> &_rrecv_msg_ptr,
		solid::ErrorConditionT const &_rerror
	);
	
	void onMessage(
		solid::frame::mpipc::ConnectionContext &_rctx,
		std::shared_ptr<RegisterResponse> &_rsent_msg_ptr,
		std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
		solid::ErrorConditionT const &_rerror
	);
	
	void onMessage(
		solid::frame::mpipc::ConnectionContext &_rctx,
		std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
		std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
		solid::ErrorConditionT const &_rerror
	);
private:
	
	uint32_t registerConnection(
		solid::frame::mpipc::ConnectionContext &_rctx,
		ConnectionData &_rcon_data,
		const RegisterRequest &_rreq
	);
	
	void unregisterConnection(ConnectionData &_rcon_data);
	
	uint32_t createNewColour(const size_t _room_index);
private:
	struct Data;
	Data &d;
};

}//namespace server
}//namespace bubbles


#endif
