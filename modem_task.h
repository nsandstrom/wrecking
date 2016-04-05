class Modem_task{
private:
	enum status {new, pending, done};
	String data;
	String reply;
	enum task {get_time, send_data, send_status};
public:
	bool has_new();
}

bool Modem_task::has_new (){
	if ( status == new ) {
		return true;
	}
	return false;
}