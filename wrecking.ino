// Base station for wrecking
#include "pinout.h"
#include "datatypes.h"
#include "modem_task.h"
#include "secrets.h"
#include <FastLED.h>
#include <Keypad.h>

#define ONE_SECOND 1000
#define CAPTURE_TIME 6

#define DEBUG

Modem_task modem_task;

Owner global_owner = neutral;
States global_state = idle; 
States global_next_state = idle; 

unsigned long global_loop_start_time;
unsigned long global_one_second_timer;

int global_next_round_countdown = 0;
int global_capture_countdown = 0;
byte global_boost = 97;

int global_input_coordinates = 0;

bool global_interrupts_locked = false;

void setup(){
	init_station();
	init_display();
	init_leds();
	init_keypad();
	init_modem();

	//debug stuff, remove later
	global_next_state = active;
	global_owner = hjortkloe;
	global_next_round_countdown = 300;
}


void loop(){
	// Sample the loop start time
	global_loop_start_time = millis();
	
	update_timers();
	station();
	modem();
	display_update();
	leds_update();


  //if next state is different from the current state, set the new state
  if (global_next_state != global_state){
    update_state();
  }
}

void init_station(){
	//Empty until needed
}

void station() {
  switch (global_state)
  {
    case idle:
      #ifdef DEBUG 
    	debug_keypad_switch_state(); 
    	#endif
      break;

    case active:
      #ifdef DEBUG 
    	debug_keypad_switch_state(); 
    	#endif
      break;

    case capturing:
      #ifdef DEBUG 
    	debug_keypad_switch_state(); 
    	#endif

      if (global_capture_countdown <= 0)
      {
        global_next_state = waitForCoordinates;
      }
      break;

    case waitForCoordinates:
      keypad_select_owner();
      break;

    case changeOwner:
      if (modem_task.task == set_owner){
        if (modem_task.completed()){
          modem_task.clear_task();
        	global_next_state = active;
        }
      }
      else{
        if (!modem_task.busy()){
         modem_task.setOwner(global_owner);
        }
      }
      break;
  }
}

void update_state(){
	
	//do state specific enter tasks
	switch (global_next_state)
	{
    case idle:
      break;

    case active:
      leds_reset_broken_sections();
      break;

		case capturing:
			global_capture_countdown = CAPTURE_TIME;
			break;

    case waitForCoordinates:
      leds_turn_off_all();
      break;

    case changeOwner:
      // start owner change using task modem_task
      if (!modem_task.busy()){
    	 modem_task.setOwner(global_owner);
      }
      break;
	}

  global_state = global_next_state;
  display_enter_state();
}

void update_timers(){
  if ((global_loop_start_time - global_one_second_timer) > ONE_SECOND)
  {
  	debug_update_boost();
    global_next_round_countdown --;
    global_capture_countdown --;
    global_one_second_timer = global_loop_start_time;
  }
}

void keypad_select_owner(){
  char key = getButton();
  bool newOwnerSelected = true;
  if (key) // Check for a valid key.
  {
    switch (key)
    {
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
        newOwnerSelected = false;
        break;
    }
    if (newOwnerSelected){
      global_next_state = changeOwner;
    }
  }
}

void debug_keypad_switch_state(){
  char key = getButton();
  if (key) // Check for a valid key.
  {
    switch (key)
    {
      case 'A':
        global_next_state = idle;
        break;
      case 'B':
        global_next_state = active;
        break;
      case 'C':
        global_next_state = capturing;
        break;
      case 'D':
        global_next_state = waitForCoordinates;
        break;
      default:
        break;
    }
	}
}

void debug_update_boost(){
	// byte direction;
	// direction = random(3);
	// if (direction == 0)
	// 	global_boost --;
	// else if (direction == 2)
	// 	global_boost ++;

  if (modem_task.task == get_boost && modem_task.completed()){
    global_boost = modem_task.get_reply();
    modem_task.clear_task();
  }
}