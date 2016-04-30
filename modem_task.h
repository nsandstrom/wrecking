#include <Arduino.h>
class Modem_task{
private:
	enum Status {New, Pending, Done, Failed};
	Status status;
	String data;
	String reply;
	enum Task {get_time, send_data, send_status};
public:
	bool has_new();
};

bool Modem_task::has_new (){
	if( status == New ) {
		return true;
	}
	return false;
}