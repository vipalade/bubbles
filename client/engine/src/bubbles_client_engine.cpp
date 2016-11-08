#include "client/engine/bubbles_client_engine.hpp"
#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"

#include "solid/frame/reactorcontext.hpp"
#include "solid/system/debug.hpp"
#include "solid/system/cassert.hpp"
#include "solid/utility/any.hpp"
#include "solid/utility/event.hpp"

#include <queue>

using namespace solid;
using namespace std;

namespace bubbles{
namespace client{

using EventQueueT = queue<Event>;

struct Engine::Data{
	Data(
		solid::frame::ServiceT &_rsvc,
		solid::frame::mpipc::Service &_rmpipc,
		const EngineConfiguration &_cfg
	):	rmpipc(_rmpipc), service(_rsvc), cfg(_cfg), push_eventq_idx(0), pop_eventq_idx(1),
		push_messageq_idx(0), pop_messageq_idx(1), discarded_on_push(false), rgb_color(0){}
	
	void discardPopEventQ(){
		EventQueueT &eq = eventq[pop_eventq_idx];
		while(eq.size()) eq.pop();
	}
	
	frame::mpipc::Service					&rmpipc;
	
	frame::ServiceT							&service; 
	string									server_endpoint;
	string									room_name;
	EventQueueT								eventq[2];
	EngineConfiguration						cfg;
	
	size_t									push_eventq_idx;
	size_t									pop_eventq_idx;
	
	size_t									push_messageq_idx;
	size_t									pop_messageq_idx;
	
	bool 									discarded_on_push;
	uint32_t								rgb_color;
	Event									last_event;
	std::shared_ptr<EventsNotification>		events_message_ptr;
	ExitFunctionT							exit_function;
	GuiUpdateFunctionT						gui_update_function;
};

Engine::Engine(
	solid::frame::ServiceT &_rsvc, solid::frame::mpipc::Service &_rmpipc,
	const EngineConfiguration &_cfg
):d(*(new Data(_rsvc, _rmpipc, _cfg))){}

Engine::~Engine(){
	delete &d;
}

solid::ErrorConditionT Engine::start(
	SchedulerT &_rsched,
	const std::string &_server_endpoint,
	const std::string &_room_name,
	uint32_t _rgb_color
){
	idbg("");
	solid::ErrorConditionT					err;
	
	if(not this->isRunning()){
		solid::DynamicPointer<frame::Object>	objptr(this);//its save - pointer count is kept by this pointer
		_rsched.startObject(objptr, d.service, solid::generic_event_category.event(solid::GenericEvents::Start), err);
		if(not err){
			d.server_endpoint = _server_endpoint;
			d.room_name = _room_name;
		}
	}
	return err;
}

void Engine::moveEvent(int _x, int _y){
	
	idbg(_x<<':'<<_y);
	
	std::unique_lock<std::mutex>	lock(d.service.mutex(*this));
	EventQueueT 					&reventq = d.eventq[d.push_eventq_idx];
	
	if((reventq.size() + 1) == d.cfg.max_event_queue_size){
		reventq.pop();
		d.discarded_on_push = true;
	}
	reventq.push(Event());
	reventq.back().type = Event::PointerMove;
	reventq.back().x = _x;
	reventq.back().y = _y;
	if(reventq.size() == 1){
		d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Raise));
	}
}

void Engine::onEvent(frame::ReactorContext &_rctx, solid::Event &&_uevent) /*override*/{
	idbg(" event = "<<_uevent);
	if(generic_event_category.event(GenericEvents::Start) == _uevent){
		
		d.events_message_ptr = std::make_shared<EventsNotification>();
		
	}else if(generic_event_category.event(GenericEvents::Kill) == _uevent){
		postStop(_rctx);
	}else if(generic_event_category.event(GenericEvents::Raise) == _uevent){
		doTrySendEvents();
	}
}

void Engine::doTrySendEvents(std::shared_ptr<EventsNotification> &&_rrecv_msg_ptr /*= std::shared_ptr<EventsNotification>{}*/){
	idbg("");
	size_t		pop_eventq_idx = 0;
	{
		std::unique_lock<std::mutex> lock(d.service.mutex(*this));
		if(_rrecv_msg_ptr){
			d.events_message_ptr = std::move(_rrecv_msg_ptr);
		}
		if(not d.events_message_ptr){
			//the message is ready to fill - see which of the eventq we should use
			if(d.discarded_on_push){
				//the push eventq is already rotating, drop what remains on pop_eventq
				d.discardPopEventQ();
				d.discarded_on_push = false;
				swap(d.pop_eventq_idx, d.push_eventq_idx);
				pop_eventq_idx = d.pop_eventq_idx;
			}else if(d.eventq[d.pop_eventq_idx].size()){
				//still have envents on popq
				pop_eventq_idx = d.pop_eventq_idx;
			}else if(d.eventq[d.push_eventq_idx].size()){
				//have events on pushq
				swap(d.pop_eventq_idx, d.push_eventq_idx);
				pop_eventq_idx = d.pop_eventq_idx;
			}else{
				pop_eventq_idx = -1;
			}
		}else{
			pop_eventq_idx = -1;
		}
	}
	if(pop_eventq_idx < 2){
		//we can fill events_message_ptr and send it to server
		EventQueueT &reventq = d.eventq[pop_eventq_idx];
		
		d.last_event = reventq.back();
		
		d.events_message_ptr->main_event = reventq.front();
		reventq.pop();
		
		while(reventq.size() and d.events_message_ptr->events.size() < 1000){
			d.events_message_ptr->events.push_back(reventq.front());
			reventq.pop();
		}
		
		std::shared_ptr<EventsNotification> tmp_ptr{std::move(d.events_message_ptr)};
		d.rmpipc.sendMessage(d.server_endpoint.c_str(), tmp_ptr);
	}
}

void Engine::onConnectionStart(solid::frame::mpipc::ConnectionContext &_rctx){
	idbg(_rctx.recipientId());
	auto msg_ptr = std::make_shared<RegisterRequest>(d.room_name, d.rgb_color);
	_rctx.service().sendMessage(_rctx.recipientId(), msg_ptr, 0|frame::mpipc::MessageFlags::WaitResponse);
}

void Engine::onConnectionStop(solid::frame::mpipc::ConnectionContext &_rctx){
	idbg(_rctx.recipientId());
	if(d.events_message_ptr){
		//connection stopped and there is no activity to send, resend the last event
		d.events_message_ptr->main_event = d.last_event;
		
		std::shared_ptr<EventsNotification> tmp_ptr{std::move(d.events_message_ptr)};
		
		d.rmpipc.sendMessage(d.server_endpoint.c_str(), tmp_ptr);
	}
}

void Engine::doSetExitFunction(ExitFunctionT &&_uf){
	d.exit_function = std::move(_uf);
}
void Engine::doSetGuiUpdateFunction(GuiUpdateFunctionT &&_uf){
	d.gui_update_function = std::move(_uf);
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<RegisterRequest> &_rsent_msg_ptr,
	std::shared_ptr<RegisterResponse> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
	
	if(_rrecv_msg_ptr and _rrecv_msg_ptr->success()){
		d.rgb_color = _rrecv_msg_ptr->rgb_color;
		
		//after activation, the mpipc will start sending pending EventsNotification messages
		_rctx.service().connectionNotifyEnterActiveState(_rctx.recipientId());
	}else if(_rrecv_msg_ptr){
		//failed registering the connection
		edbg(_rctx.recipientId()<<" Connection registration failed because ["<<_rrecv_msg_ptr->message<<"]. Exiting");
		d.exit_function();
	}
}

void Engine::onMessage(
	solid::frame::mpipc::ConnectionContext &_rctx,
	std::shared_ptr<EventsNotification> &_rsent_msg_ptr,
	std::shared_ptr<EventsNotification> &_rrecv_msg_ptr,
	solid::ErrorConditionT const &_rerror
){
	idbg(_rctx.recipientId()<<" error: "<<_rerror.message());
	
	if(_rsent_msg_ptr){
		_rsent_msg_ptr->clear();
		doTrySendEvents(std::move(_rsent_msg_ptr));
	}
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
