//This will config the sim 9000
#include <SoftwareSerial.h>

#define GRPS_BAUD 38400
#define SERIAL_BAUD 115200

#define CHECK_CONNECTION_INTERVAL 600000

#define SERIAL_DEBUG
#ifdef SERIAL_DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif

SoftwareSerial GPRS(MODEM_PIN_TX, MODEM_PIN_RX);

// unsigned char buffer[64]; // buffer array for data recieve over serial port   ANVÄNDS DENNA?
int buffer_count=0;     // buffer_counter for buffer array 
bool GPRS_online = false;
String APN;
unsigned long lockTime = 0;


enum Gprs_responses
{
	OK,
	Any,
	Accepted,
	None,
	Message,
	IP,
	Number
};

void init_modem(){
	pinMode(MODEM_PIN_RST, OUTPUT);
	digitalWrite(MODEM_PIN_RST, HIGH);
	digitalWrite(MODEM_PIN_RST, LOW);
	delay(1000);
	digitalWrite(MODEM_PIN_RST, HIGH);
	GPRS.begin(GRPS_BAUD);               // the GPRS baud rate
	#ifdef SERIAL_DEBUG
		Serial.begin(SERIAL_BAUD);
		Serial.println(F("Serial interface started"));
	#endif
	DEBUG_PRINT(F("GPRS interface started"));

	bool init_completed = false;
	unsigned long timeout = 0;

	while(!init_completed){
		lock_interrupts();	
		readBack(None);
		for (int i=1; i <= 10; i++){
			GPRS.write("AT\r\n");
			readBack(None);
			delay(1);
		}

		delay(5000);
		readBack(None);
		DEBUG_PRINT(F("Check AT"));	
		for (int i=1; i <= 3; i++){
			GPRS.println("AT");
		  	while (readBack(Any) != 1);
		}
		

		String Carrier = getCarrier();
		DEBUG_PRINT((String)F("Post getcarrier: ") + Carrier);
		APN = getAPN(Carrier);
		DEBUG_PRINT((String)F("Apn: ") + APN);
		delay(2000);
		readBack(None);
		readBack(None);
		readBack(None);

		unlock_interrupts();

		DEBUG_PRINT(F("Modem init completed"));
		init_completed = true;
	}
}

void modem(){
	//Chose active main task  
	
//Test code for interrupt flag
	static unsigned long check_connection_timeout = millis();
	static unsigned long check_boost_timeout = millis() + 10000;
	if (check_connection_timeout < millis()){
		GPRS_online = false;
		check_connection_timeout = millis() + CHECK_CONNECTION_INTERVAL;
	}

	if (check_boost_timeout < millis()){
		if (!modem_task.busy()) {
			DEBUG_PRINT(F("Time to check boost"));
			modem_task.getBoost();
			check_boost_timeout = millis() + 5000;
		}
	}

	if (modem_task.has_new()){
		DEBUG_PRINT("there is new task");
		modem_task.begin_task();
	}

	if (GPRS_online){
		
		if (modem_task.pending()){
			task_send_data(modem_task.data);			
		}
	}
	else{
		task_connect();
	}
}

void task_nothing(){
}

void task_connect(){
	static byte state = 0, next_state = 8, retry_state = 0, error_count = 5, answer = 0;
	static Gprs_responses expectAnswer = None;
	static unsigned long wait_until = 0, timeout = 0;
	static bool allowLeds = false;
	String apnSet;

	if (wait_until < millis() && !allowLeds ){
	
		// DEBUG_PRINT((String)F("expexts answer: ") + (String)expectAnswer);
		DEBUG_PRINT((String)F("state: ") + (String)state);

		answer = readBack(expectAnswer);
		if ( answer == 1 ) {
			unlock_interrupts();
			allowLeds = false;
			state = next_state;
			DEBUG_PRINT((String)F("run connection state: ") + (String)state);
			switch(state) {
				case 0:
				case 1:			//Send AT 3 times
				case 2:
					GPRS.println(F("AT"));
					lock_interrupts();
					expectAnswer = OK;
					timeout = millis() + 100;					
				  	next_state = state + 1;
				break;
				case 3:			//Send CGATT=1
					GPRS.println(F("AT+CGATT=1"));
					lock_interrupts();
					expectAnswer = OK;
					timeout = millis() + 100;					
					next_state = 4;
					retry_state = 0;
				break;
				case 4:
					GPRS.println(F("AT+CGATT?"));
					lock_interrupts();
					expectAnswer = OK;
					timeout = millis() + 100;
					wait_until = millis() + 5;
					next_state = 5;
					retry_state = 0;
				break;
				case 5:
					GPRS.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
					lock_interrupts();
					expectAnswer = OK;
					timeout = millis() + 100;
					wait_until = millis() + 10;
					next_state = 6;
					retry_state = 0;
				break;
				case 6:
					apnSet = (String)F("AT+SAPBR=3,1,\"APN\",\"") + APN + "\"";
					GPRS.println(apnSet);
					lock_interrupts();
					expectAnswer = OK;
					timeout = millis() + 100;
					wait_until = millis() + 10;
					next_state = 7;
					retry_state = 0;
				break;
				case 7:
					GPRS.println(F("AT+SAPBR=1,1"));
					lock_interrupts();
					expectAnswer = Any;
					timeout = millis() + 200;
					wait_until = millis() + 10;
					next_state = 8;
					retry_state = 0;
				break;
				case 8:
					expectAnswer = None;
					wait_until = millis() + 1000;
					next_state = 9;
				break;
				case 9:
					readBack(None);
					GPRS.println(F("AT+SAPBR=2,1"));
					DEBUG_PRINT(F("printing: AT+SAPBR=2,1"));
					lock_interrupts();
					expectAnswer = IP;
					timeout = millis() + 1000;
					wait_until = millis() + 10;
					next_state = 10;
					retry_state = 0;
				break;
				case 10:			//Last state in task. Success
					GPRS_online = true;
					next_state = 8;
					expectAnswer = None;
					DEBUG_PRINT(F("Test cycle completed"));
				break;
				case 98:
					next_state = 99;
					expectAnswer = None;
					digitalWrite(MODEM_PIN_RST, LOW);
					wait_until = millis() + 100;
					DEBUG_PRINT(F("Restarting modem"));
				break;
				case 99:
					next_state = 0;
					expectAnswer = None;
					digitalWrite(MODEM_PIN_RST, HIGH);
					wait_until = millis() + 25000;		//Wait for signal
				break;
			}
		}
		else if( timeout < millis() || answer == 2 ){
			unlock_interrupts();
			DEBUG_PRINT(F("Goto retry state"));
			next_state = retry_state;
			expectAnswer = None;
			error_count--;
		}

		if (error_count <= 0){
			error_count = 5;
			wait_until = millis() + 1000;
			expectAnswer = None;
			next_state = 98;
		}
	}
	else if(allowLeds){
		DEBUG_PRINT(F("Allowing leds to update"));
		allowLeds = false;

	}
	
}

void task_send_data(String data){
	static byte state = 0, next_state = 0, retry_state = 0, error_count = 5;
	static Gprs_responses expectAnswer = None;
	static unsigned long wait_until = 0, timeout = 0;
	// String URL = SERVER_URL + "/2/so?key=" + SERVER_KEY + ";owner=" + (String)3;
	String URL;

	if (wait_until < millis() ){
	
		// DEBUG_PRINT((String)F("expexts answer: ") + (String)expectAnswer);
		DEBUG_PRINT((String)F("state: ") + (String)state);
		int answer = readBack(expectAnswer);
		if ( answer == 1 ) {
			unlock_interrupts();
			state = next_state;
			DEBUG_PRINT((String)F("running SO state: ") + (String)state);
			switch(state) {
				case 0:		//Test connection
					readBack(None);
					GPRS.println(F("AT+SAPBR=2,1"));
					DEBUG_PRINT(F("printing: SAPBR=2,1"));
					lock_interrupts();
					expectAnswer = IP;
					timeout = millis() + 1000;
					wait_until = millis() + 10;
					next_state = 1;
					retry_state = 0;
				break;
				case 1:
					expectAnswer = None;
					next_state = 2;
				break;
				case 2:		//HTTP Init
					readBack(None);
					GPRS.println(F("AT+HTTPINIT"));
					DEBUG_PRINT(F("printing: HTTPINIT"));
					lock_interrupts();
					expectAnswer = Any;
					timeout = millis() + 1000;
					wait_until = millis() + 10;
					next_state = 3;
					retry_state = 1;
				break;
				case 3:
					expectAnswer = None;
					next_state = 4;
				break;
				case 4:		//HTTP params
					readBack(None);
					if (modem_task.task == set_owner){
						URL = SERVER_URL + (String)"2" + (String)F("/so?key=") + SERVER_KEY + (String)F(";owner=") + data;
					}
					else if (modem_task.task = get_boost){
						URL = SERVER_URL + (String)"2" + (String)F("/gb");
					}
					DEBUG_PRINT(URL);
					GPRS.println((String)F("AT+HTTPPARA=\"URL\",\"") + (String)URL + "\"");
					DEBUG_PRINT(F("printing: HTTPPARA"));
					lock_interrupts();
					expectAnswer = Any;
					timeout = millis() + 1000;
					wait_until = millis() + 10;
					next_state = 5;
					retry_state = 1;
				break;
				case 5:
					expectAnswer = None;
					next_state = 6;
				break;
				case 6:		//HTTP Action
					readBack(None);
					GPRS.println(F("AT+HTTPACTION=0"));
					DEBUG_PRINT(F("printing: HTTPACTION=0"));
					lock_interrupts();
					expectAnswer = OK;
					timeout = millis() + 1000;
					wait_until = millis() + 10;
					next_state = 7;
					retry_state = 6;
				break;
				case 7:
					expectAnswer = None;
					// lock_interrupts();
					timeout = millis() + 3000;
					wait_until = millis() + 3000;
					next_state = 8;
				break;
				case 8:
					expectAnswer = None;
					next_state = 9;
				break;
				case 9:		//HTTP Action
					GPRS.println(F("AT+HTTPREAD"));
					DEBUG_PRINT(F("printing: HTTPREAD"));
					lock_interrupts();
					if (modem_task.task == set_owner){
						expectAnswer = Accepted;
					}
					else{
						expectAnswer = Number;
					}
					timeout = millis() + 3000;
					wait_until = millis() + 10;
					next_state = 98;
					retry_state = 6;
				break;
				case 98:		//HTTP terminate
					readBack(None);
					GPRS.println(F("AT+HTTPTERM"));
					DEBUG_PRINT(F("printing: HTTPTERM"));
					lock_interrupts();
					expectAnswer = Any;
					timeout = millis() + 1000;
					wait_until = millis() + 10;
					next_state = 99;
					retry_state = 0;
				break;
				case 99:
					modem_task.complete_task();
					expectAnswer = None;
					next_state = 0;
					error_count = 5;
				break;
			}
		}
		else if( timeout < millis() || answer == 2 ){
			unlock_interrupts();
			DEBUG_PRINT(F("Goto retry state"));
			next_state = retry_state;
			expectAnswer = None;
			error_count--;
		}

		if (error_count <= 0){
			error_count = 5;
			wait_until = millis() + 1000;
			expectAnswer = None;
			next_state = 0;
			GPRS_online = false;
		}
	}
	
}

void task_get_tts(){
	
}



//Init functions
String getCarrier(){
	while (true){
		GPRS.println(F("AT+COPS?"));
		DEBUG_PRINT(F("Asked for carrier info"));
		// GPRS.write("AT+COPS?\r");
		// GPRS.write((const char*)F("AT+COPS?\r"));
		
		// while(!GPRS.available()){}
			delay(100);
		while(GPRS.available()){
			String message = GPRS.readString();
				DEBUG_PRINT((String)F("full reply: ") + message);
			String carrier;
			if (message.substring(11,22) == F("+COPS: 0,0,")) {
				carrier = message.substring(message.indexOf("\"")+1, message.lastIndexOf("\""));				
				DEBUG_PRINT((String)F("Carrier: ") + carrier);
				return carrier;
			}
			else{				
				DEBUG_PRINT(F("identifying carrier"));
			}
		}
		delay(500);
	}
}

String getAPN(String carrier){
	if (carrier == F("Tele2")) {
		return F("4G.tele2.se");
	}
	else if (carrier == F("Telia") || carrier == F("TELIA MOBILE")) {
		return F("online.telia.se");
	}
	else {
		return F("NO APN");
	}
}

//Common functions


int readBack(int expected){
	DEBUG_PRINT((String)F("readback looking for: ") + (String)expected);
	int responded = 0;				// 0 = none, 1 = expected, 2 = else
	static char response[64];
	memset(response, '\0', 64);    // Initialize the string
	if ( expected == None ){
		responded = 1;
	}
	if (GPRS.available())              // if date is comming	 from softwareserial port ==> data is comming from gprs shield
		{
		delay(10);
		while(GPRS.available())          // reading data into char array 
			{
			// delay(1);
			response[buffer_count++]=GPRS.read();     // writing data into array
			if(buffer_count == 64)break;
		}
		DEBUG_PRINT((String)F("count after response ") + (String)buffer_count);            // if no data transmission ends, write buffer to hardware serial port
		DEBUG_PRINT("Response: " + (String)response);            // if no data transmission ends, write buffer to hardware serial port
		// DEBUG_PRINT(F("response of some kind"));            // if no data transmission ends, write buffer to hardware serial port
		if (expected == Any){
			responded = 1;
		}
		if ( expected == OK && strstr(response, "OK") ){
			responded = 1;
			DEBUG_PRINT(F("this was an OK command"));
		}
		if ( expected == Accepted && strstr(response, "Ok") ){
			responded = 1;
			DEBUG_PRINT(F("this was an Accepted command"));
		}
		if ( (expected == OK || expected == Number ) && strstr(response, "ERROR") ){
			responded = 2;
			DEBUG_PRINT(F("Responded with ERROR"));
		}
		if ( expected == IP ) {
			if ( strstr(response, "OK") && !strstr(response, "0.0.0.0") ){
			// if ( (expected == IP && strstr(response, "OK") && !strstr(response, "0.0.0.0") )){
				responded = 1;
				DEBUG_PRINT(F("Responded with non zero IP"));
			}
			else {
				DEBUG_PRINT(F("IP error"));
				responded = 2;
			}
		}
		if ( expected == Number && strstr(response, "Ok:") ){
			DEBUG_PRINT(F("Responded with number"));
			char* split; 
			strtok(response, ":");
			split = strtok(NULL, ":");
			split = strtok(NULL, ":");
			modem_task.set_reply(split);
			DEBUG_PRINT("split:" + (String)split);
			// modem_task.set_reply((String)response.substring(3));
			responded = 1;
			DEBUG_PRINT("stored answer: " + (String)modem_task.get_reply());
		}
		buffer_count = 0;                       // set counter of while loop to zero
	}
	return responded;
}

void lock_interrupts(){
	lockTime = millis();
	global_interrupts_locked = true;
	DEBUG_PRINT(F("Interrupts locked."));
}

void unlock_interrupts(){
	if (global_interrupts_locked){
		global_interrupts_locked = false;
		DEBUG_PRINT("Interrupts UNlocked. -----------------> " + (String)(millis() - lockTime));
	}
}