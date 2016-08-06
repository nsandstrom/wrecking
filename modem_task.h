#include <Arduino.h>
enum Modem_Tasks {get_time, get_boost, set_owner, send_status};

class Modem_task {
private:
	enum Status {New, Pending, Done, Failed, None};
	Status status = None;
	String reply;
	
	
public:
	bool has_new();
	bool begin_task();
	bool complete_task();
	bool pending();
	bool completed ();
	void setOwner (int owner);
	String data;
	
	Modem_Tasks task;
	
};


//for modem code
bool Modem_task::has_new (){
	if( status == New ) {
		return true;
	}
	return false;
}

bool Modem_task::begin_task (){
	status = Pending;
}

bool Modem_task::complete_task (){
	status = Done;
}




//For main code
bool Modem_task::completed (){
	if (status == Done){
		return true;
	}
	return false;
}

bool Modem_task::pending (){
	if (status == Pending){
		return true;
	}
	return false;
}

void Modem_task::setOwner (int owner) {
	task = set_owner;
	data = (String)owner;
	status = New;
	//change Status and Task
} 