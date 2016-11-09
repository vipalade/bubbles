#include "client/engine/bubbles_client_engine.hpp"
#include "solid/frame/mpipc/mpipcservice.hpp"
#include "solid/frame/mpipc/mpipcconfiguration.hpp"

#include "solid/frame/reactorcontext.hpp"
#include "solid/system/debug.hpp"
#include "solid/system/cassert.hpp"
#include "solid/utility/any.hpp"
#include "solid/utility/event.hpp"

#include <queue>
#include <deque>
using namespace solid;
using namespace std;

namespace bubbles{
namespace client{

using EventQueueT = queue<Event>;

using EventsNotificationDequeT = deque<std::shared_ptr<EventsNotification>>;

struct PlotStub{
	uint32_t	rgb_color;
	int32_t		x;
	int32_t		y;
	string		text;
	size_t		pending_pos;
	bool		ploted;
};

using PlotStubDequeT = deque<PlotStub>;
using EventStubDequeT = deque<EventStub>;

struct Engine::Data{
	Data(
		solid::frame::ServiceT &_rsvc,
		solid::frame::mpipc::Service &_rmpipc,
		const EngineConfiguration &_cfg
	):	rmpipc(_rmpipc), service(_rsvc), cfg(_cfg), push_eventq_idx(0), pop_eventq_idx(1),
		push_messagedq_idx(0), pop_messagedq_idx(1), read_plotdq_idx(0), write_plotdq_idx(1), read_plotdq_count(0),
		discarded_on_push(false), rgb_color(0){}
	
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
	
	size_t									push_messagedq_idx;
	size_t									pop_messagedq_idx;
	
	size_t									read_plotdq_idx;
	size_t									write_plotdq_idx;
	size_t									read_plotdq_count;
	
	bool 									discarded_on_push;
	uint32_t								rgb_color;
	Event									last_event;
	std::shared_ptr<EventsNotification>		events_message_ptr;
	ExitFunctionT							exit_function;
	GuiUpdateFunctionT						gui_update_function;
	EventsNotificationDequeT				messagedq[2];
	PlotStubDequeT							plotdq[2];
	EventStubDequeT							event_stubdq;
};


//=============================================================================
PlotIterator::PlotIterator(PlotIterator &&_plotit):reng(_plotit.reng){
	plotdq_index = _plotit.plotdq_index;
	pos = _plotit.pos;
	_plotit.plotdq_index = solid::InvalidIndex();
	rgb_color = _plotit.rgb_color;
}

PlotIterator::~PlotIterator(){
	if(plotdq_index != solid::InvalidIndex()){
		std::unique_lock<std::mutex>	lock(reng.d.service.mutex(reng));
		--reng.d.read_plotdq_count;
	}
}

uint32_t PlotIterator::rgbColor()const{
	return reng.d.plotdq[plotdq_index][pos].rgb_color;
}

int32_t PlotIterator::x()const{
	return reng.d.plotdq[plotdq_index][pos].x;
}
int32_t PlotIterator::y()const{
	return reng.d.plotdq[plotdq_index][pos].y;
}

const std::string& PlotIterator::text()const{
	return reng.d.plotdq[plotdq_index][pos].text;
}

bool PlotIterator::end()const{
	return reng.d.plotdq[plotdq_index].size() == pos;
}

PlotIterator& PlotIterator::operator++(){
	++pos;
	return *this;
}

PlotIterator::PlotIterator(Engine &_reng):reng(_reng){
	pos = 0;
	std::unique_lock<std::mutex>	lock(reng.d.service.mutex(reng));
	plotdq_index = reng.d.read_plotdq_idx;
	++reng.d.read_plotdq_count;
	rgb_color = reng.d.rgb_color;
}

//=============================================================================
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

PlotIterator Engine::plot(){
	return PlotIterator(*this);
}

void Engine::moveEvent(int _x, int _y){
	
	idbg(_x<<':'<<_y);
	bool notify_engine = false;
	{
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
		notify_engine = (reventq.size() == 1);
	}
	if(notify_engine){
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
	}else if(generic_event_category.event(GenericEvents::Message) == _uevent){
		doProcessIncomingNotifications(_rctx);
	}
}

void Engine::doProcessIncomingNotifications(solid::frame::ReactorContext &_rctx){
	{
		std::unique_lock<std::mutex> lock(d.service.mutex(*this));
		swap(d.pop_messagedq_idx, d.push_messagedq_idx);
	}
	
	EventsNotificationDequeT	&rmessagedq{d.messagedq[d.pop_messagedq_idx]};
	
	//put all the event stubs in a queue
	for(auto& msg_ptr:rmessagedq){
		d.event_stubdq.push_back(std::move(msg_ptr->event_stub));
		for(auto &e_s: msg_ptr->event_stubs){
			d.event_stubdq.push_back(std::move(e_s));
		}
	}
	
	rmessagedq.clear();
	
	auto& rplotdq = d.plotdq[d.write_plotdq_idx];
	
	const size_t qsz = d.event_stubdq.size();
	for(size_t i = 0; i < qsz; ++i){
		auto&		revent_stub = d.event_stubdq.front();
		
		bool		drop_event_stub = false;
		
		auto bin_srch_ret = solid::binary_search(
			rplotdq.begin(), rplotdq.end(), revent_stub.sender_rgb_color,
			[](const PlotStub &_rps, const uint32_t _key){
				if(_key < _rps.rgb_color) return -1;
				if(_key > _rps.rgb_color) return 1;
				return 0;
			}
		);
		
		if(bin_srch_ret.first){//entry found
			
			PlotStub &rps = rplotdq[bin_srch_ret.second];
			
			if(rps.ploted){
				
				//we can change to a new event
				if(rps.pending_pos < (revent_stub.events.size() + 1)){
					Event &revt = rps.pending_pos == 0 ? revent_stub.event : revent_stub.events[rps.pending_pos];
					
					if(revt.type == Event::Unknown){//entry must be erased
						rplotdq.erase(rplotdq.begin() + bin_srch_ret.second);
						drop_event_stub = true;
					}else{
						rps.ploted = false;
						rps.text = revent_stub.text;
						rps.x = revt.x;
						rps.y = revt.y;
						++rps.pending_pos;
						if(rps.pending_pos >= (revent_stub.events.size() + 1)){
							drop_event_stub = true;
						}
					}
				}
			}
		}else{//entry not found - insert it
			Event &revt = revent_stub.event;
			if(revt.type != Event::Unknown){//entry must be erased
				PlotStub &rps = *(rplotdq.insert(rplotdq.begin() + bin_srch_ret.second, PlotStub{}));
				rps.rgb_color = revent_stub.sender_rgb_color;
				rps.text = revent_stub.text;
				rps.x = revt.x;
				rps.y = revt.y;
				rps.pending_pos = 1;
				rps.ploted = false;
				if(rps.pending_pos >= (revent_stub.events.size() + 1)){
					drop_event_stub = true;
				}
			}else{
				drop_event_stub = true;
			}
		}
		
		if(not drop_event_stub){
			d.event_stubdq.push_back(std::move(revent_stub));
		}
		d.event_stubdq.pop_front();
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
		if(d.events_message_ptr){
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
		
		d.events_message_ptr->event_stub.event = reventq.front();
		reventq.pop();
		
		while(reventq.size() and d.events_message_ptr->event_stub.events.size() < 1000){
			d.events_message_ptr->event_stub.events.push_back(reventq.front());
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
	idbg(_rctx.recipientId()<<' '<<_rctx.error().message());
	if(d.events_message_ptr){
		//connection stopped and there is no activity to send, resend the last event
		d.events_message_ptr->event_stub.event = d.last_event;
		
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
		
		idbg(_rctx.recipientId()<<"MY COLOR: "<<d.rgb_color);
		
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
	if(_rrecv_msg_ptr){
		bool notify = false;
		{
			std::unique_lock<std::mutex>	lock(d.service.mutex(*this));
			EventsNotificationDequeT		&rmessagedq = d.messagedq[d.push_messagedq_idx];
			
			rmessagedq.push_back(std::move(_rrecv_msg_ptr));
			notify = (rmessagedq.size() == 1);
		}
		
		if(notify){
			d.service.manager().notify(d.service.manager().id(*this), generic_event_category.event(GenericEvents::Message));
		}
	}else if(_rsent_msg_ptr){
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
