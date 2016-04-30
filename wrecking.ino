// Base station for wrecking
#include "pinout.h"
#include "modem_task.h"
#include "datatypes.h"
#include <FastLED.h>
#include <Keypad.h>

Modem_task modem_task ;

Owner global_owner = neutral;
States global_state = idle; 

unsigned long global_loop_start_time;

void setup(){
	init_station();
	init_display();
	init_leds();
	init_keypad();
	init_modem();

	//debug stuff, remove later
	global_state = active;
	global_owner = hjortkloe;
}


void loop(){
	// Sample the loop start time
	global_loop_start_time = millis();
	station();
	modem();
	display_update();
	leds_update();
}

void init_station(){
	//Empty until needed
}

void station() {
	debug_keypad_switch_state();
}

void debug_keypad_switch_state(){
  char key = getButton();
  if (key) // Check for a valid key.
  {
    switch (key)
    {
      case 'A':
        global_state = idle;
        break;
      case 'B':
        global_state = active;
        break;
      case 'C':
        global_state = capturing;
        break;
      case 'D':
        global_state = waitForCoordinates;
        break;
      case '1':
        global_owner = kaos;
        break;
      case '2':
        global_owner = cybercom;
        break;
      case '3':
        global_owner = klustret;
        break;
      case '4':
        global_owner = hjortkloe;
        break;
      default:
        break;
    }
	}
}