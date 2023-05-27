const byte KEYPAD_ROWS = 4; // Four rows
const byte KEYPAD_COLS = 4; // Three columns
// Define the Keymap
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins.
byte rowPins[KEYPAD_ROWS] = { KEYPAD_ROW0, KEYPAD_ROW1, KEYPAD_ROW2, KEYPAD_ROW3 };
// Connect keypad COL0, COL1 and COL2 to these Arduino pins.
byte colPins[KEYPAD_COLS] = { KEYPAD_COL0, KEYPAD_COL1, KEYPAD_COL2, KEYPAD_COL3 };

//declare the global keypad variable
Keypad global_kpd = Keypad( makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS );

void init_keypad() {
	// Create the Keypad
	global_kpd = Keypad( makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS );
}

char getButton() {

  char key = global_kpd.getKey();
  return key;
}

