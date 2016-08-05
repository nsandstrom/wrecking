//This will config the sim 9000
#include <SoftwareSerial.h>

#define GRPS_BAUD 38400
#define SERIAL_BAUD 115200

#define CHECK_CONNECTION_INTERVAL 60000

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
	IP
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
	
	readBack(None);
	for (int i=1; i <= 10; i++){
		GPRS.write("AT\r\n");
		delay(1);
	}

	delay(5000);

	DEBUG_PRINT(F("Check AT"));
	lock_interrupts();
	for (int i=1; i <= 3; i++){
		GPRS.println("AT");
	  	while (readBack(Any) != 1);
	}
	unlock_interrupts();

	String Carrier = getCarrier();
	DEBUG_PRINT((String)F("Post getcarrier: ") + Carrier);
	APN = getAPN(Carrier);
	DEBUG_PRINT((String)F("Apn: ") + APN);


}

void modem(){
	//Chose active main task  
	
//Test code for interrupt flag
	static unsigned long wait_untill = millis();
	if (wait_untill < millis()){
		GPRS_online = false;
		wait_untill = millis() + CHECK_CONNECTION_INTERVAL;
	}

	if (GPRS_online){
		task_set_owner();

	}
	else{
		task_connect();
	}
}

void task_nothing(){
}

void task_connect(){
	static byte state = 0, next_state = 8, retry_state = 0, error_count = 5;
	static Gprs_responses expectAnswer = None;
	static unsigned long wait_until = 0, timeout = 0;
	static bool allowLeds = false;
	String apnSet;

	if (wait_until < millis() && !allowLeds ){
	
		// DEBUG_PRINT((String)F("expexts answer: ") + (String)expectAnswer);
		DEBUG_PRINT((String)F("state: ") + (String)state);
		int answer = readBack(expectAnswer);
		if ( answer == 1 ) {
			unlock_interrupts();
			allowLeds = true;
			state = next_state;
			DEBUG_PRINT((String)F("rn st: ") + (String)state);
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
					next_state = 9	;
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

void task_set_owner(){
	static byte state = 0, next_state = 8, retry_state = 0, error_count = 5;
	static Gprs_responses expectAnswer = None;
	static unsigned long wait_until = 0, timeout = 0;
	static bool allowLeds = false;

	if (wait_until < millis() && !allowLeds ){
	
		// DEBUG_PRINT((String)F("expexts answer: ") + (String)expectAnswer);
		DEBUG_PRINT((String)F("state: ") + (String)state);
		int answer = readBack(expectAnswer);
		if ( answer == 1 ) {
			unlock_interrupts();
			allowLeds = true;
			state = next_state;
			DEBUG_PRINT((String)F("running state: ") + (String)state);
			switch(state) {
			}
		}
		else if( timeout < millis() || answer == 2 ){
			unlock_interrupts();
			DEBUG_PRINT(F("Goto retry state"));
			next_state = retry_state;
			expectAnswer = None;
			error_count--;
		}
	}
	else if(allowLeds){
		DEBUG_PRINT(F("Allowing leds to update"));
		allowLeds = false;

	}



	
}

void task_get_tts(){
	
}



//Init functions
String getCarrier(){
	while (true){
		GPRS.println(F("AT+COPS?"));
		// GPRS.write("AT+COPS?\r");
		// GPRS.write((const char*)F("AT+COPS?\r"));
		
		while(!GPRS.available()){}
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

bool GPRS_check_online(){

}

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
		// DEBUG_PRINT((String)F("count after response ") + (String)buffer_count);            // if no data transmission ends, write buffer to hardware serial port
		DEBUG_PRINT("Response: " + (String)response);            // if no data transmission ends, write buffer to hardware serial port
		// DEBUG_PRINT(F("response of some kind"));            // if no data transmission ends, write buffer to hardware serial port
		if (expected == Any){
			responded = 1;
		}
		if ( expected == OK && strstr(response, "OK") ){
			responded = 1;
			DEBUG_PRINT(F("this was an OK command"));
		}
		if ( (expected == OK || expected == Message) && strstr(response, "ERROR") ){
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
		DEBUG_PRINT("Interrupts UNlocked. - " + (String)(millis() - lockTime));
	}
}