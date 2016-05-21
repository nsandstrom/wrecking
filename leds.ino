#define NUM_LEDS 32
#define MAX_BRIGHT 64
#define LOW_BRIGHT 32
#define LEDS_MAX_FADE_BRIGHT 20
#define LEDS_MIN_FADE_BRIGHT 4

#define SECTIONS 4
#define SECTION_LENGTH NUM_LEDS/SECTIONS

#define LED_UPDATE_INTERVAL 200
#define LED_FADE_INTERVAL 100

//this variable keeps track if leds shall be redrawn
bool redrawLeds = true;

//define LED array
CRGB leds[NUM_LEDS];

//define wrecker colors
CHSV colorNeutral(0, 0, MAX_BRIGHT);
CHSV colorKaos(0, 255, MAX_BRIGHT);
CHSV colorCyberCom(160, 255, MAX_BRIGHT);
CHSV colorKlustret(200, 255, MAX_BRIGHT);
CHSV colorHjortkloe(32, 255, MAX_BRIGHT);
CHSV colorBlack(0, 0, 0);

CHSV led_colors[6] = {colorNeutral,
                      colorKaos,
                      colorCyberCom,
                      colorKlustret,
                      colorHjortkloe,
                      colorBlack
                     };

void init_leds() {
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);
  FillAll(led_colors[global_owner], LOW_BRIGHT);
  FastLED.show();
}

void leds_update() {
  static unsigned long led_animate_time;
  switch (global_state)
  {
    case idle:
      if ((global_loop_start_time - led_animate_time) > LED_FADE_INTERVAL)
      {
        leds_fade();
        led_animate_time = global_loop_start_time;
      }
      break;

    case active:
      if ((global_loop_start_time - led_animate_time) > LED_UPDATE_INTERVAL)
      {
        leds_animate_running();
        led_animate_time = global_loop_start_time;
      }

      break;

    default:
      break;
  }


  if (redrawLeds) {
    FastLED.show();
    redrawLeds = false;
  }
}

void leds_fade() {
  static byte brightness = 0;
  static bool count_up = true;

  FillAll(led_colors[global_owner], brightness);

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
  FillAll(led_colors[global_owner], LOW_BRIGHT);

  for (int section = 0; section < SECTIONS; section++) {
    int sectionOffset;
    int sectionDot;
    int sectionPredot;
    int sectionAfterDot;

    //First calculate dot position for the different section
    //if odd section number, reverse flow
    if (section == 2 || section == 3) {
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

    //Then update LEDS
    leds[sectionPredot] = led_colors[global_owner];
    leds[sectionPredot] %= 100;

    leds[sectionDot] = led_colors[global_owner];

    leds[sectionAfterDot] = led_colors[global_owner];
    leds[sectionAfterDot] %= 100;
  }

  //increment and wrap dot
  dot++;
  if (dot >= SECTION_LENGTH)
    dot = 0;

  //redraw leds at the end of next loop
  redrawLeds = true;
}

void FillAll(CHSV color, byte brightness) {
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
    static bool flashing = false;

    //if flashing, dim and stop flash
    if (flashing) {
      FillSection(colorNeutral, (64), section);
      flashing = false;
    } //If not currently flashing, randomize for flash
    else {
      int randNumber = random(300);
      if (randNumber > 295) {
        FillSection(colorNeutral, MAX_BRIGHT, section);
      }
      else if (randNumber > 275) {
        FillSection(colorBlack, 0, section);
      }
    }
  }
}

