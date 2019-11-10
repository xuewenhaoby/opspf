#ifndef __MESSAGA_H__
#define __MESSAGE_H__

#define SET(a,_a) a = _a
#define GET(a) return a
#define MESSAGE_FREE(m) delete m; m = NULL

enum EventType{
	MSG_NODE_Test,
	MSG_NODE_Initialize,
	MSG_NODE_Finalize,
	MSG_NODE_UpdateLink,
	
	MSG_LINK_Initialize,
	MSG_LINK_Finalize,
	
	// MSG_SAT_UpdatePos,
	
	MSG_TIME_Timer,

	MSG_CTRL_Empty,
	MSG_CTRL_Stop,
};

class Message{
public:
	Message(){}
	Message(void *_node,EventType _type){
		SET(node,_node);SET(type,_type);
	}
	~Message(){}

	void * getNode(){GET(node);}
	EventType getEventType(){GET(type);}
	void setEventType(EventType _type){SET(type,_type);}
private:
	void *node;
	EventType type;
};

#endif