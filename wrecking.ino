// Base station for wrecking
#include "pinout.h"
#include "datatypes.h"
#include "modem_task.h"
#include "secrets.h"
#include <FastLED.h>
#include <Keypad.h>

#define ONE_SECOND 1000
#define UPDATE_TIME_INTERVAL 10
#define UPDATE_BOOST_INTERVAL 10

#define CAPTURE_TIME 60

Modem_task modem_task;

Owner global_owner = neutral;
States global_state = idle; 
States global_next_state = idle; 

unsigned long global_loop_start_time = 0;
unsigned long global_one_second_timer = 0;
int global_update_boost_timer = 0;
int global_update_time_timer = 0;

long global_time = 0;
int global_capture_countdown = 0;
byte global_boost = 97;

int global_input_coordinates = 0;

bool global_interrupts_locked = false;

void setup(){
  int signalQuality = 0;

	init_station();
	init_display();
	init_leds();
	init_keypad();
	signalQuality = init_modem();

  get_initial_data(signalQuality);

  // make sure the initial state is entered propperly
  update_state();
}

void get_initial_data(int signalQuality){
  char downloadItem = 0;

  display_dwlding_data(signalQuality);

  //run this until all data is downloaded
  while (downloadItem < 3){
    
    //start a new task if modem not busy
    if (!modem_task.busy()) 
    {
      switch (downloadItem)
      {
        case 0:
          display_print(F("Boost level"), 1, 0);
          modem_task.getBoost();
          break;
        case 1:
          display_print(F("Timing information"), 1, 0);
          modem_task.getTime();
          break;
        case 2:
          display_print(F("Ownership status  "), 1, 0);
          modem_task.getOwner();
          break;
      }
      
    }
    //check task results if modem completed
    else if (modem_task.completed()){
      update_with_latest_modem_data();
      downloadItem++;
    }
    else
    {
      //wait untill task is completed
      while (!modem_task.completed())
      {
        modem();
      }
    } 
  }
}


void loop(){
	// Sample the loop start time
	global_loop_start_time = millis();
	
	update_timers();
	station();
	modem();
	display_update();
	leds_update();


  //check task results if modem has completed a task
  if (modem_task.completed()){
    update_with_latest_modem_data();
  }

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
      //go to active if global time < 0
      if (global_time < 0)
        {
          global_next_state = active;
        }

        request_timning_information();

      break;

    case active:
      //go to idle if global time > 0
      if (global_time > 0)
      {
        global_next_state = idle;
      }

      request_timning_information();
      request_boost_information();

      //check if someone wants to capture the station
      keypad_start_capture();
      break;

    case capturing:
      if (global_capture_countdown <= 0)
      {
        global_next_state = waitForCoordinates;
      }

      //check if someone wants to abort the capture
      keypad_abort_capture();

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
        // start owner change using task modem_task
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
      break;
	}

  global_state = global_next_state;
  display_enter_state();
}

void update_timers(){
  if ((global_loop_start_time - global_one_second_timer) > ONE_SECOND)
  {
    global_time --;
    global_capture_countdown --;
    global_update_boost_timer++;
    global_update_time_timer++;
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

void keypad_start_capture(){
  char key = getButton();
  if (key) // Check for a valid key.
  {
    switch (key)
    {
      case '*':
      case 'C':
        global_next_state = capturing;
        break;

      default:
        break;
    }
  }
}

void keypad_abort_capture(){
  char key = getButton();
  if (key) // Check for a valid key.
  {
    switch (key)
    {
      case '*':
      case 'A':
        global_next_state = active;
        break;

      default:
        break;
    }
  }
}

//Updates the right variable with the latest modem data
//Check if task completed before running this function
void update_with_latest_modem_data(){

  //put the data where it belongs
  switch (modem_task.task)
  {
    case get_boost:
      global_boost = modem_task.get_reply();
      //clear the task NOTE, THIS MUST BE ON EVERY CASE
      modem_task.clear_task();
      break;

    case get_time:
      global_time = modem_task.get_reply();
      //clear the task NOTE, THIS MUST BE ON EVERY CASE
      modem_task.clear_task();
      break;

    case get_owner:
      global_owner = (Owner)modem_task.get_reply();
      //clear the task NOTE, THIS MUST BE ON EVERY CASE
      modem_task.clear_task();
      break;

    default:
      //DONT CLEAR THE TASK HERE, LET SOMETHING ELSE DO THAT
      break;
  }
}


void request_timning_information(){
  //Update the timing information if modem is not bussy and the intervall is right
  if (!modem_task.busy() && global_update_time_timer > UPDATE_TIME_INTERVAL)
  {
    modem_task.getTime();

    //reset the timer
    global_update_time_timer = 0;
  }
}
void request_boost_information(){
  //Update the timing information if modem is not bussy and the intervall is right
  if (!modem_task.busy() && global_update_boost_timer > UPDATE_BOOST_INTERVAL)
  {
    modem_task.getBoost();

    //reset the timer
    global_update_boost_timer = 0;
  }
}