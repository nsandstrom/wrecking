
/*
   Demo_NHD0420CW-Ax3_SPI.ino

   Tutorial sketch for use of character OLED slim display family by Newhaven with Arduino Uno, without
   using any library.  Models: NHD0420CW-Ax3, NHD0220CW-Ax3, NHD0216CW-Ax3. Controller: US2066
   in this example, the display is connected to Arduino via SPI interface.

   Displays on the OLED alternately a 4-line message and a sequence of character "block".
   This sketch assumes the use of a 4x20 display; if different, modify the values of the two variables
   ROW_N e COLUMN_N.
   The sketch uses the minimum possible of Arduino's pins; if you intend to use also /RES or /CS lines,
   the related instructions are already present, it's sufficient to remove the comment markers.

   The circuit:
   OLED pin 1 (Vss)    to Arduino pin ground
   OLED pin 2 (VDD)    to Arduino pin 5V
   OLED pin 3 (REGVDD) to Arduino pin 5V
   OLED pin 4 to 6     to Vss ground
   OLED pin 7 (SCLK)   to Arduino pin D13 (SCK)
   OLED pin 8 (SID)    to Arduino pin D11 (MOSI)
   OLED pin 9 (SOD)    to Arduino pin D12 (MISO) (optional, can be not connected)
   OLED pin 10 to 14   to Vss ground
   OLED pin 15 (/CS)   to Vss ground  (or to Arduino pin D2, in case of use of more than one display)
   OLED pin 16 (/RES)  to Arduino pin Reset or VDD 5V (or to Arduino pin D3, to control reset by sw)
   OLED pin 17 (BS0)   to Vss ground
   OLED pin 18 (BS1)   to Vss ground
   OLED pin 19 (BS2)   to Vss ground
   OLED pin 20 (Vss)   to Vss ground

   Original example created by Newhaven Display International Inc.
   Modified and adapted to Arduino Uno 30 Mar 2015 by Pasquale D'Antini
   Modified 19 May 2015 by Pasquale D'Antini
   Modified to use hardware SPI 1 April 2016 by Joakim Sandström

   This example code is in the public domain.
*/

// inslude the SPI library:
#include <SPI.h>
#include "teamInfo.h"

#define BLINK_TIME 500


const byte ROW_N = 4;                 // Number of display rows
const byte COLUMN_N = 20;             // Number of display columns

//const byte RES = 3;                 // Arduino's pin assigned to the Reset line (optional, can be always high)
const String DISPLAY_SHORT_TEAM_NAME[5] = {"     ",
                                           TEAM1SHORT,
                                           TEAM2SHORT,
                                           TEAM3SHORT,
                                           TEAM4SHORT
                                          };

const String DISPLAY_TEAM_NAME[5] =   {"            ",
                                       TEAM1NAME,
                                       TEAM2NAME,
                                       TEAM3NAME,
                                       TEAM4NAME
                                      };


byte new_line[4] = {0x80, 0xA0, 0xC0, 0xE0};               // DDRAM address for each line of the display
byte rows = 0x08;                     // Display mode: 1/3 lines or 2/4 lines; default 2/4 (0x08)

unsigned long display_blink_timer =0;
bool display_blink = true;

void init_display(void)                      // INITIAL SETUP
{
  //   pinMode(RES, OUTPUT);            // Initializes Arduino pin for the Reset line (optional)
  //   digitalWrite(RES, HIGH);         // Sets HIGH the Reset line of the display (optional, can be always high)
  SPI.begin();
  SPI.setBitOrder(LSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE3);

  delayMicroseconds(200);            // Waits 200 us for stabilization purpose


  if (ROW_N == 2 || ROW_N == 4)
    rows = 0x08;                    // Display mode: 2/4 lines
  else
    rows = 0x00;                    // Display mode: 1/3 lines

  command(0x22 | rows); // Function set: extended command set (RE=1), lines #
  command(0x71);        // Function selection A:
  data(0x5C);           //  enable internal Vdd regulator at 5V I/O mode (def. value) (0x00 for disable, 2.8V I/O)
  command(0x20 | rows); // Function set: fundamental command set (RE=0) (exit from extended command set), lines #
  command(0x08);        // Display ON/OFF control: display off, cursor off, blink off (default values)
  command(0x22 | rows); // Function set: extended command set (RE=1), lines #
  command(0x79);        // OLED characterization: OLED command set enabled (SD=1)
  command(0xD5);        // Set display clock divide ratio/oscillator frequency:
  command(0x70);        //  divide ratio=1, frequency=7 (default values)
  command(0x78);        // OLED characterization: OLED command set disabled (SD=0) (exit from OLED command set)

  if (ROW_N > 2)
    command(0x09);  // Extended function set (RE=1): 5-dot font, B/W inverting disabled (def. val.), 3/4 lines
  else
    command(0x08);  // Extended function set (RE=1): 5-dot font, B/W inverting disabled (def. val.), 1/2 lines

  command(0x06);        // Entry Mode set - COM/SEG direction: COM0->COM31, SEG99->SEG0 (BDC=1, BDS=0)
  command(0x72);        // Function selection B:
  data(0x0A);           //  ROM/CGRAM selection: ROM C, CGROM=250, CGRAM=6 (ROM=10, OPR=10)
  command(0x79);        // OLED characterization: OLED command set enabled (SD=1)
  command(0xDA);        // Set SEG pins hardware configuration:
  command(0x10);        //  alternative odd/even SEG pin, disable SEG left/right remap (default values)
  command(0xDC);        // Function selection C:
  command(0x00);        //  internal VSL, GPIO input disable
  command(0x81);        // Set contrast control:
  command(0x7F);        //  contrast=127 (default value)
  command(0xD9);        // Set phase length:
  command(0xF1);        //  phase2=15, phase1=1 (default: 0x78)
  command(0xDB);        // Set VCOMH deselect level:
  command(0x40);        //  VCOMH deselect level=1 x Vcc (default: 0x20=0,77 x Vcc)
  command(0x78);        // OLED characterization: OLED command set disabled (SD=0) (exit from OLED command set)
  command(0x20 | rows); // Function set: fundamental command set (RE=0) (exit from extended command set), lines #
  command(0x01);        // Clear display
  delay(2);             // After a clear display, a minimum pause of 1-2 ms is required
  command(0x80);        // Set DDRAM address 0x00 in address counter (cursor home) (default value)
  command(0x0C);        // Display ON/OFF qcontrol: display ON, cursor off, blink off
  delay(250);           // Waits 250 ms for stabilization purpose after display on

  if (ROW_N == 2)
    new_line[1] = 0xC0;             // DDRAM address for each line of the display (only for 2-line mode)


  delay(250);           // Waits 250 ms for stabilization purpose after display on
  display_print(F("--------------------"), 0, 0);
  display_print(F("Starting station"), 1, 0);
  display_print(STATION_ID, 1, 17);
  display_print(F("Please wait ..."), 3, 0);
}

void display_dwlding_data(int signalQuality){
  command(0x01);        // Clear display
  delay(2);             // After a clear display, a minimum pause of 1-2 ms is required


  display_print(F("Downloading"), 0, 0);
  display_print(F("Signal strength:"), 2, 0);
  display_print_value(signalQuality, 2, 2, 17, ' ');
  display_print(F("Please wait ..."), 3, 0);  
}

void display_timeing_update(){
  command(0x01);        // Clear display
  delay(2);             // After a clear display, a minimum pause of 1-2 ms is required

  display_print(F("Synchronizing"), 0, 0);
  display_print(F("timing information"), 1, 0);
}

void display_update () {
  switch (global_state)
  {
    case idle:
      if (global_time != 999999){
        display_print_HHMMSS(global_time, 2, 0);  
 
        //display_print_value(global_time, 6, 3,0, ' ');
      }
  
      break;

    case active:
      // station is owned by special team
      if (global_owner == SPECIALTEAM){
        display_blinkText(F("     ATTENTION!     "), 0, 0);
        display_blinkText(F(" Lantern siezed by: "), 1, 0);
        display_blinkText(F("        403         "), 2, 0);
      }
      //else continue as normal
      else{
        //blink boost if it deviates from 100
        if (global_boost != 100){
          if (display_blink){
            display_print_value(global_boost, 3, 0, 16, ' ');
            display_print(F("%"), 0, 19);
          }
          else {
            display_print(F("    "), 0, 16);
          }
        }
        else{
          display_print_value(global_boost, 3, 0, 16, ' ');
          display_print(F("%"), 0, 19);
        }
        display_print(DISPLAY_TEAM_NAME[global_owner], 2,7);

        display_print_HHMMSS(global_time*-1, 1, 11);  
      }
      break;

    case capturing:
      display_blinkText(F("Reinitializing"), 0, 0);

      display_progress(2, (20-global_capture_countdown/(CAPTURE_TIME/20)));
      
      break;

    case enterCalibration:
      display_global_input(1, 0, false);
      break;

    case selectTeam: 
      break;

    case verifyCalibration:
      display_blinkText(F("Verifying"), 0, 0);
      break;

    default:
      break;
  }

  if ((global_loop_start_time - display_blink_timer) >= BLINK_TIME)
  {
    display_blink = !display_blink;
    display_blink_timer = global_loop_start_time;
  }
}

// sets display values
void display_enter_state(){

  command(0x01);        // Clear display
  delay(2);             // After a clear display, a minimum pause of 1-2 ms is required
  // turn offcursor
  command(0x0C);        // display ON, cursor off, blink off

  switch(global_state)
  {
    case idle:
      display_print(F("ID:"), 0,0);
      display_print(STATION_ID, 0, 3);
      display_print(F("Next COM-WIN:"), 1,0);
      if (global_time == 999999)
        display_print(F("UNKNOWN"), 2,0);
      display_print(F("Press C to calibrate"), 3,0);
      break;

    case active:
      display_print(F("ID:"), 0,0);
      display_print(STATION_ID, 0, 3);
      display_print(F("Output:    %"), 0, 8);
      display_print(F("Time left:"), 1, 0);
      display_print(F("Owner:"), 2, 0);
      
      display_print(F("Press C to change"), 3, 0);
      break;

    case capturing:
      display_print(F("Reinitializing"), 0, 0);      
      display_print(F("Please wait ..."), 1, 0);  
      display_print(F("Press A to abort"), 3, 0);  
      break;

    case selectTeam:
      display_print(F("Select new owner:"), 0, 0);
      
      display_print("1.", 2, 0);
      display_print(DISPLAY_SHORT_TEAM_NAME[1], 2,2);     
      display_print("2.", 2, 10);
      display_print(DISPLAY_SHORT_TEAM_NAME[2], 2,12);

      display_print("3.", 3, 0);
      display_print(DISPLAY_SHORT_TEAM_NAME[3], 3,2);
      display_print("4.", 3, 10);
      display_print(DISPLAY_SHORT_TEAM_NAME[4], 3,12);


      break;

    case changeOwner:
      display_print(F("Uploading new"), 0, 0);      
      display_print(F("ownership data"), 1, 0);      
      display_print(F("Please wait ..."), 2, 2);      
      break;

    case enterCalibration:
      display_print(F("Enter code: "), 0, 0);      
      display_print(F("C=Confirm, A=Abort"), 3, 0);
      display_global_input(1,0, true);
      break;

    case verifyCalibration:
      display_print(global_input_string,1,0);
      display_print(F("Please wait ..."), 2, 0);
      break;

    case verifyFail:
      display_print(F("ERROR"), 0, 0);
      display_print(global_input_string,1,0);
      display_print(F("Is not a valid code"), 2, 0);
      display_print(F("Press C to continue"), 3, 0);
      break;

    case verifySucess:
      display_print(F("Verification sucess"), 0, 0);
      display_print(global_input_string,1,0);
      display_print(F("Is a valid code"), 2, 0);
      display_print(F("Press C to continue"), 3, 0);
      break;


    default:
      break;
  }

  //reset the blinking so text is always showed state changes
  display_blink = true;
  display_blink_timer = global_loop_start_time;
}

// _______________________________________________________________________________________
void command(byte c)                  // SUBROUTINE: PREPARES THE TRANSMISSION OF A COMMAND
{
  SPI.transfer(0x1F);

  send_byte(c);                      // Transmits the byte
}


// _______________________________________________________________________________________
void data(byte d)
{
  SPI.transfer(0x5F);

  send_byte(d);
}

// _______________________________________________________________________________________
void send_byte(byte tx_b)
{
  //Split the bytes into two and pad the last four bits with 0s
  byte tx_b1 = tx_b & 0x0F;
  byte tx_b2 = (tx_b >> 4) & 0x0F;

  //Or together the bytes
  int tx_int = (tx_b2 << 8) | tx_b1;

  //transfer it
  SPI.transfer16(tx_int);
}

// _______________________________________________________________________________________

//displays a right-adjusted value
void display_print_value(long value, byte number_of_digits, byte row, byte start, char fillCharacter){
  String value_string;
  byte offset;

  value_string = String(value);
  offset = number_of_digits - value_string.length();
  command(new_line[row]+start);           //  moves the cursor to the first column of that line

  for (byte c = 0; c < number_of_digits; c++)  // One character at a time,
  {
    if (c < offset)
      data(fillCharacter);
    else
      data(value_string[c-offset]);
  }
}

//displays a integer as HHMMSS
void display_print_HHMMSS(long value, byte row, byte start){

  long HH = value/3600;
  int MM = (value - (HH*3600))/60;
  int SS = value - (HH*3600) - (MM*60);

  //display hours
  display_print_value(HH, 2, row, start, '0');
  display_print(F(":"), row, start+2);
  //display minutes
  display_print_value(MM, 2, row, start+3, '0');
  display_print(":", row, start+5);
  //display Seconds
  display_print_value(SS, 2, row, start+6, '0');
}

void display_print(String text, byte row, byte start){
  byte text_length = text.length();
  command(new_line[row]+start);           //  moves the cursor to the first column of that line
  for (byte c = 0; c < text_length; c++)  // One character at a time,
  {
    data(text[c]); 
  }
}

void display_progress(byte row, byte progress){
  command(new_line[row]);           //  moves the cursor to the first column of that line
  //avoid display overflow
  if (progress > 20)
    progress = 20;
  for (byte c = 0; c < progress; c++)  // One character at a time,
  {
    data(0xFC); 
  }

}

// displays blinking text, should be put in display update
void display_blinkText(String text, byte row, byte start){
  byte text_length = text.length();
  command(new_line[row]+start);           //  moves the cursor to the first column of that line
  if (display_blink){
    for (byte c = 0; c < text_length; c++)  // One character at a time,
    {
      data(text[c]); 
    }
  }
  else {
        for (byte c = 0; c < text_length; c++)  // One character at a time,
    {
      data(' '); 
    }
  }
}

void display_global_input(byte row, byte col, bool forceDraw){
  static int last_input_pointer = 0;
  if ((last_input_pointer != global_input_pointer) or (forceDraw)){
    command(0x0C);        // display ON, cursor off, blink off
    display_print(global_input_string,row,col);
    command(new_line[1]+global_input_pointer);           //  moves the cursor to the first column of that line
    command(0x0F);        // display ON, cursor ON, blink ON
    last_input_pointer = global_input_pointer;
  }
}