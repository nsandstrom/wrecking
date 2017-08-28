#include "teamInfo.h"


#define NUM_LEDS 144
#define MAX_BRIGHT 255
#define FLASH_BRIGHT 180
#define LOW_BRIGHT 32
#define LEDS_MAX_FADE_BRIGHT 32
#define LEDS_MIN_FADE_BRIGHT 1

#define SECTIONS 8
#define SECTION_LENGTH NUM_LEDS/SECTIONS

#define LED_UPDATE_INTERVAL 50
#define LED_FADE_INTERVAL 100
#define LED_FLASH_INTERVAL 100

//this variable keeps track if leds shall be redrawn
bool redrawLeds = true;

//this array keeps track of if a section is flashing or blacked out
//0 means that the section is operating normaly
static byte led_sections_broken[SECTIONS];

//define LED array
CRGB leds[NUM_LEDS];

//define wrecker colors
CHSV colorNeutral(0, 0, 100);
CHSV colorPlayer1(TEAM1HUE, 255, MAX_BRIGHT);
CHSV colorPlayer2(TEAM2HUE, 255, MAX_BRIGHT);
CHSV colorPlayer3(TEAM3HUE, 255, MAX_BRIGHT);
CHSV colorPlayer4(TEAM4HUE, 255, MAX_BRIGHT);
CHSV colorBlack(0, 0, 0);

CHSV led_colors[6] = {colorNeutral,
                      colorPlayer1,
                      colorPlayer2,
                      colorPlayer3,
                      colorPlayer4,
                      colorBlack
                     };

void init_leds() {
  //make sure all sections start att unbroken  
  leds_reset_broken_sections();
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);
  leds_fill_all(led_colors[global_owner], LOW_BRIGHT);
  FastLED.show();
}

void leds_update() {
  static unsigned long led_animate_time = 0;
  static char flash_divider = 0;
  switch (global_state)
  {
    case idle:
    default: 
      if ((global_loop_start_time - led_animate_time) > LED_FADE_INTERVAL)
      {
        leds_fade();
        led_animate_time = global_loop_start_time;
      }
      break;

    case active:
    case startCapture:
      if ((global_loop_start_time - led_animate_time) > LED_UPDATE_INTERVAL)
      {
        leds_animate_running();
        led_animate_time = global_loop_start_time;
      }
      break;

    case capturing:
      if ((global_loop_start_time - led_animate_time) > LED_UPDATE_INTERVAL)
      {
        leds_animate_running();

        // dont flash on every cycle
        if (flash_divider == 0)
        {
          FlashLedRow();
          flash_divider = 1;
        }
        else
        {
          flash_divider--;
        }
        
        led_animate_time = global_loop_start_time;
      }
      break;

    case selectTeam:
      break;
  }


  if (redrawLeds && !global_interrupts_locked) {
    FastLED.show();
    redrawLeds = false;
  }
}

void leds_fade() {
  static byte brightness = 0;
  static bool count_up = true;

  leds_fill_all(led_colors[global_owner], brightness);

  if (count_up) {
    brightness ++;
  } else {
    brightness --;
  }
  if ( brightness == LEDS_MAX_FADE_BRIGHT) {
    count_up = false;
  } else if ( brightness == LEDS_MIN_FADE_BRIGHT) {
    count_up = true;
  }
}

void leds_animate_running() {
  static int dot = 0;
  int preDot;
  int afterDot;

  //set pre and after-dot
  preDot = dot + 1;
  afterDot = dot - 1;
  //if dot is at the last position, set predot to the first
  if (dot >= (SECTION_LENGTH - 1))
    preDot = 0;
  //if dot is at the first position, set afterdot to the last
  else if (dot == 0)
    afterDot = SECTION_LENGTH - 1;

  //reset all colors
  //leds_fill_all(led_colors[global_owner], LOW_BRIGHT);

  for (int section = 0; section < SECTIONS; section++) {
    int sectionOffset;
    int sectionDot;
    int sectionPredot;
    int sectionAfterDot;

    //only update section if it is not broken
    if (led_sections_broken[section]==0)
    {
      //First calculate dot position for the different section
      //if odd section number, reverse flow
      if (section == 4 || section == 5|| section == 6|| section == 7) {
        sectionOffset = ((section + 1) * SECTION_LENGTH) - 1;

        //preglow
        sectionPredot = sectionOffset - preDot;

        //mainGLow
        sectionDot = sectionOffset - dot;

        //afterglow
        sectionAfterDot = sectionOffset - afterDot;
      }
      //if even section number, let the flow be normal
      else {
        sectionOffset = section * SECTION_LENGTH;

        //preglow
        sectionPredot = sectionOffset + preDot;

        //mainGLow
        sectionDot = sectionOffset + dot;

        //afterglow
        sectionAfterDot = sectionOffset + afterDot;
      }

      //reset the entire section
      FillSection(led_colors[global_owner], LOW_BRIGHT, section);

      //Then update LEDS
      leds[sectionPredot] = led_colors[global_owner];
      leds[sectionPredot] %= 100;

      leds[sectionDot] = led_colors[global_owner];

      leds[sectionAfterDot] = led_colors[global_owner];
      leds[sectionAfterDot] %= 100;
    }
  }

  //increment and wrap dot
  dot++;
  if (dot >= SECTION_LENGTH)
    dot = 0;

  //redraw leds at the end of next loop
  redrawLeds = true;
}

void leds_fill_all(CHSV color, byte brightness) {
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = color;
    leds[dot] %= brightness;
  }

  //redraw leds at the end of next loop
  redrawLeds = true;
}

void FillSection (CHSV color, byte brightness, byte section) {
  int sectionDot;

  for (int dot = 0; dot < SECTION_LENGTH; dot++) {
    sectionDot = SECTION_LENGTH * section + dot;
    leds[sectionDot] = color;
    leds[sectionDot] %= brightness;
  }

  //redraw leds at the end of next loop
  redrawLeds = true;
}

void FlashLedRow() {
  

  for (byte section = 0; section < SECTIONS; section++) {
    //if led_sections_broken, indicates a flashing situation, dim and set to 0
    if (led_sections_broken[section]>100) {
      FillSection(colorNeutral, 127, section);
      led_sections_broken[section] = 0;
    } 
    //if led_sections_broken indicates an outage, decrement outage counter
    else if (led_sections_broken[section]>0){
      led_sections_broken[section]--;
    }
    //If not currently active, randomize for flash
    else {
      int randNumber = random(300);
      if (randNumber > (180+(global_capture_countdown*2))) {
        
        //four in one triggers should flash, the rest should be blacked out
        if ((randNumber&0x0F) == 0){
          led_sections_broken[section] = 101;
          FillSection(colorNeutral, FLASH_BRIGHT, section);
        }
        else
          led_sections_broken[section] = 3;
          FillSection(colorBlack, 0, section);
      }
    }
  }
}

//reset all broken sections
void leds_reset_broken_sections(){
  for (int i = 0; i < SECTIONS; ++i)
  {
    led_sections_broken[i] = 0;
  }
}

void leds_turn_off_all(){
  leds_fill_all(led_colors[5], 0);
}

