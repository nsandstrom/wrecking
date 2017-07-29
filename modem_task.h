#include <Arduino.h>
enum Modem_Tasks {none, get_time, get_boost, set_owner, get_owner, send_status, set_under_capture, verify_calibration_code, submit_calibration_code};

class Modem_task {
private:
	enum Status {New, Pending, Done, Failed, None};
	Status status = None;
	String reply;
	
	
public:
	bool has_new();
	bool begin_task();
	void complete_task();
	void clear_task();
	bool pending();
	bool completed();
	bool busy();
	void set_reply(String reply);
	long get_reply();
	void setOwner (int owner);
	void getBoost ();
	void getOwner ();
	void getTime ();
	void setUnderCapture ();
	void verifyCalibrationCode (String code);
	void submitCalibrationCode (String code);
	String data;
	
	Modem_Tasks task = none;
	
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

void Modem_task::complete_task (){
	status = Done;
}

void Modem_task::clear_task (){
	status = None;
	task = none;
}




//For main code
bool Modem_task::completed (){
	if (status == Done){
		return true;
	}
	return false;
}

bool Modem_task::busy (){
	if (status == None){
		return false;
	}
	return true;
}

bool Modem_task::pending (){
	if (status == Pending){
		return true;
	}
	return false;
}

long Modem_task::get_reply() {
	return atol(reply.c_str());
}

void Modem_task::set_reply (String newReply) {
	reply = newReply;
	//change Status and Task
}

void Modem_task::setOwner (int owner) {
	task = set_owner;
	data = (String)owner;
	status = New;
	//change Status and Task
}

void Modem_task::getBoost () {
	task = get_boost;
	status = New;
	//change Status and Task
}

void Modem_task::getOwner () {
	task = get_owner;
	status = New;
	//change Status and Task
}

void Modem_task::getTime () {
	task = get_time;
	status = New;
	//change Status and Task
}

void Modem_task::setUnderCapture () {
	task = set_under_capture;
	status = New;
	//change Status and Task
}

void Modem_task::verifyCalibrationCode (String code) {
	task = verify_calibration_code;
	data = owner;
	status = New;
	//change Status and Task
}

void Modem_task::submitCalibrationCode (String code) {
	task = submit_calibration_code;
	data = owner;
	status = New;
	//change Status and Task
}
