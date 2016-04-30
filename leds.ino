//Fix FastLed
#include <FastLED.h>

#define NUM_LEDS 300
#define MAX_BRIGHT 255
#define LOW_BRIGHT 16

#define SECTIONS 4
#define SECTION_LENGTH NUM_LEDS/SECTIONS

//this variable keeps track if leds shall be redrawn
bool redrawLeds = true;

//define LED array
CRGB leds[NUM_LEDS];

//define wrecker colors
CHSV colorKaos(0, 255, MAX_BRIGHT);
CHSV colorCyberCom(160, 255, MAX_BRIGHT);
CHSV colorKlustret(200, 255, MAX_BRIGHT);
CHSV colorHjortkloe(32, 255, MAX_BRIGHT);
CHSV colorNeutral(0, 0, MAX_BRIGHT);
CHSV colorBlack(0, 0, 0);
CHSV ownerColor1 = colorNeutral;


void init_leds() {
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);
  FastLED.show();
  ownerColor1 = colorKaos;
  FillAll(ownerColor1, LOW_BRIGHT);
  FastLED.show();
}

void RedrawLeds() {
  if (redrawLeds) {
    redrawLeds = false;
    FastLED.show();
  }
}

void AnimateLed() {
  static int dot = 0;
  int preDot = dot + 1;
  int afterDot = dot - 1;
  int lastDot = dot - 2;

  //reset all colors
  FillAll(ownerColor1, LOW_BRIGHT);

  for (int section = 0; section < SECTIONS; section++) {
    int sectionOffset;
    int sectionDot;
    int sectionPredot;
    int sectionAfterDot;
    int sectionLastDot;

    //First calculate dot position for the different section
    //if odd section number, revverse flow
    if (section & 0x01) {
      sectionOffset = (section + 1) * SECTION_LENGTH - 1;

      //preglow
      sectionPredot = sectionOffset - preDot;

      //mainGLow
      sectionDot = sectionOffset - dot;

      //afterglow
      sectionAfterDot = sectionOffset - afterDot;

      //lastGlow
      sectionLastDot = sectionOffset - lastDot;
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

      //lastGlow
      sectionLastDot = sectionOffset + lastDot;
    }

    //Then update LEDS

    //only draw preglow if within bounds
    if (preDot < SECTION_LENGTH) {
      leds[sectionPredot] = ownerColor1;
      leds[sectionPredot] %= 100;
    }

    //only draw mainglow if within bounds
    if (dot < SECTION_LENGTH) {
      leds[sectionDot] = ownerColor1;
    }

    //only draw afterglow if within bounds
    if (afterDot >= 0) {
      leds[sectionAfterDot] = ownerColor1;
      leds[sectionAfterDot] %= 100;
    }

    //only draw Lastdot if within bounds
    if (lastDot >= 0) {
      leds[sectionLastDot] = ownerColor1;
      leds[sectionLastDot] %= LOW_BRIGHT;
    }
    

  }

  dot++;
  if (dot > SECTION_LENGTH)
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

void ChangeOwnerColor(void) {
  CHSV color;
  switch (g_owner) {
    case neutral:
      ownerColor1 = colorNeutral;
      break;
    case kaos:
      ownerColor1 = colorKaos;
      break;
    case cybercom:
      ownerColor1 = colorCyberCom;
      break;
    case klustret:
      ownerColor1 = colorKlustret;
      break;
    case hjortkloe:
      ownerColor1 = colorHjortkloe;
      break;
  }
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

