//This will config the sim 9000
#include <SoftwareSerial.h>

#define GRPS_BAUD 9600
#define SERIAL_BAUD 19200

#define SERIAL_DEBUG
#ifdef SERIAL_DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif

SoftwareSerial GPRS(MODEM_PIN_TX, MODEM_PIN_RX);

unsigned char buffer[64]; // buffer array for data recieve over serial port   ANVÄNDS DENNA?
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
	Message
};

void init_modem(){
	GPRS.begin(GRPS_BAUD);               // the GPRS baud rate
	#ifdef SERIAL_DEBUG
		Serial.begin(SERIAL_BAUD);
		Serial.println(F("Serial interface started"));
	#endif
	DEBUG_PRINT(F("GPRS interface started"));

	lock_interrupts();
	for (int i=1; i <= 3; i++){
		GPRS.write("AT\r\n");
	  	while (!readBack(OK));
	}
	unlock_interrupts();

	// String Carrier = getCarrier();
	// DEBUG_PRINT((String)F("Post getcarrier: ") + Carrier);
	// APN = getAPN(Carrier);
	// DEBUG_PRINT("Apn: " + APN);


}

void modem(){
	//Chose active main task  
	
//Test code for interrupt flag
	static unsigned long deadline = millis();
	if (deadline < millis()){
		GPRS_online = false;
		deadline = millis() + 2000;
	}

	if (GPRS_online){

	}
	else{
		task_connect();
	}
}

void task_nothing(){
}

void task_connect(){
	static int state = 0, next_state = 0;
	static Gprs_responses expectAnswer = None;

	
	DEBUG_PRINT("expect: " + (String)expectAnswer);
	DEBUG_PRINT("state: " + (String)state);
	if ( readBack(expectAnswer) ) {
		unlock_interrupts();
		// state = next_state;
		DEBUG_PRINT("running state: " + (String)state);
		switch(state) {
			case 0:
			case 1:
			case 2:
				GPRS.write("AT\r\n");
				expectAnswer = OK;
				lock_interrupts();
			  	// while (!readBack(OK));
			  	state++;
			break;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
				GPRS.println("AT+CGMI");
				// GPRS.println("AT+CGATT=1");
				expectAnswer = OK;
				lock_interrupts();
				next_state = 4;
				state++;
				// while (!readBack(OK));
			break;
			case 14:
				GPRS_online = true;
				state = 0;
				expectAnswer = None;
				DEBUG_PRINT(F("Test cycle completed"));
			break;
		}
	}
	
	
}

void task_set_owner(){
	static int state = 0;
	static Gprs_responses expectAnswer = None;
	
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
			delay(10);
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
				DEBUG_PRINT(F("idendifying carrier"));
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

bool readBack(int expected){
	bool responded = false;	
	static char response[64];
	memset(response, '\0', 64);    // Initialize the string
	if ( expected == None ){
		responded = true;
	}
	if (GPRS.available())              // if date is comming	 from softwareserial port ==> data is comming from gprs shield
		{
		delay(10);
		while(GPRS.available())          // reading data into char array 
			{
			response[buffer_count++]=GPRS.read();     // writing data into array
			if(buffer_count == 64)break;
		}
		DEBUG_PRINT("count after response " + (String)buffer_count);            // if no data transmission ends, write buffer to hardware serial port
		DEBUG_PRINT(response);            // if no data transmission ends, write buffer to hardware serial port
		DEBUG_PRINT(F("response of some kind"));            // if no data transmission ends, write buffer to hardware serial port
		if (expected == Any){
			responded = true;
		}
		if ( expected == OK && strstr(response, "OK") ){
			responded = true;
			DEBUG_PRINT(F("this was an OK command"));
		}
		clearBufferArray();              // call clearBufferArray function to clear the storaged data from the array
		buffer_count = 0;                       // set counter of while loop to zero
	}
	return responded;
}

void clearBufferArray()              // function to clear buffer array
{
	for (int i=0; i<buffer_count;i++)
    { buffer[i]=NULL;}                  // clear all index of array with command NULL
}

void lock_interrupts(){
	lockTime = millis();
	global_interrupts_locked = true;
	DEBUG_PRINT(F("Interrupts locked."));
}

void unlock_interrupts(){
	if (global_interrupts_locked){
		global_interrupts_locked = false;
		// int time_locked = 
		DEBUG_PRINT("Interrupts UNlocked. - " + (String)(millis() - lockTime));
	}
}