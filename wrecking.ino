// Base station for wrecking
#include "pinout.h"
#include "modem_task.h"

Modem_task modem_task ;

void setup(){
	init_station();
	init_display();
	init_leds();
	init_keypad();
	init_modem();
}


void loop(){
	station();
	modem();
	display_update();
	leds_update();
}

void init_station(){
	//Empty until needed
}