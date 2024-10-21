#include <Arduino.h>
#include <IRremote.hpp>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DECODE_NEC        // Includes Apple and Onkyo. To enable all protocols , just comment/disable this line.
#define IR_RECEIVE_PIN 2  // To be compatible with interrupt example, pin 2 is chosen here.

#define NO_OF_INPUTS 25

enum InputStates {
  BTN_UP,
  BTN_DOWN,
  BTN_START_STOP,
  BTN_SET_RESET,
  IR_CH_MIN,
  IR_CH,
  IR_CH_MAX,
  IR_LEFT_SHIFT,
  IR_RIGHT_SHIFT,
  IR_START_STOP,
  IR_MIN,
  IR_MAX,
  IR_EQ,
  IR_100PLUS,
  IR_200PLUS,
  IR_0,
  IR_1,
  IR_2,
  IR_3,
  IR_4,
  IR_5,
  IR_6,
  IR_7,
  IR_8,
  IR_9
};

enum CustomChar {
  SOUND_ICON = 0,
  SOUND_MUTE_ICON = 1,
  SOUND_LOW_ICON = 2,
  SOUND_MEDIUM_ICON = 3,
  SOUND_HIGH_ICON = 4,
  TONE_ICON = 5,
  BATTERY_ICON = 6
};

enum Sound {
  MUTE,
  LOW,
  MEDIUM,
  HIGH
};

enum Tone {
    TONE_1,
    TONE_2,
    TONE_3
};

enum DisplayMode {
  DEFAULT_MODE,
  BLINK_TIMER_MODE,
  BLINK_SOUND_MODE,
  BLINK_TONE_MODE,
  RUNNING_MODE,
  PAUSE_MODE
};

unsigned long countdownTime;

const int pushButtonPins[] = { 3, 4, 5, 6 };
bool inputStates[NO_OF_INPUTS];
bool inputStatesLast[NO_OF_INPUTS];

byte lastButtonState[4];
unsigned long lastTimeButtonStateChanged[4] = { 0 };
unsigned long debounceDuration = 200; // millis

LiquidCrystal_I2C lcd(0x27, 20, 4);

//Custom Character
byte soundIcon[] = {
  B00001,
  B00011,
  B00111,
  B11111,
  B11111,
  B00111,
  B00011,
  B00001
};

byte soundMuteIcon[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte soundLowIcon[] = {
  B00000,
  B00000,
  B00000,
  B10000,
  B10000,
  B00000,
  B00000,
  B00000
};

byte soundMediumIcon[] = {
  B00000,
  B01000,
  B00100,
  B10100,
  B10100,
  B00100,
  B01000,
  B00000
};

byte soundHighIcon[] = {
  B00010,
  B01001,
  B00101,
  B10101,
  B10101,
  B00101,
  B01001,
  B00010
};

byte toneIcon[] = {
  B00100,
  B00110,
  B00111,
  B00100,
  B11100,
  B11100,
  B11100,
  B00000
};

byte batteryIcon[] = {
  B00110,
  B01111,
  B01111,
  B01111,
  B01111,
  B01111,
  B01111,
  B00000
};

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;  // Wait for Serial to become available.

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  lcd.init();
  lcd.backlight();
  lcd.createChar(SOUND_ICON, soundIcon);
  lcd.createChar(SOUND_MUTE_ICON, soundMuteIcon);
  lcd.createChar(SOUND_LOW_ICON, soundLowIcon);
  lcd.createChar(SOUND_MEDIUM_ICON, soundMediumIcon);
  lcd.createChar(SOUND_HIGH_ICON, soundHighIcon);
  lcd.createChar(TONE_ICON, toneIcon);
  lcd.createChar(BATTERY_ICON, batteryIcon);
  lcd.home();

  // Set up the pin modes for the push buttons
  for (int i = 0; i < 4; i++) {
    pinMode(pushButtonPins[i], INPUT);
    lastButtonState[i] = digitalRead(pushButtonPins[i]);  // Read initial state of each button
  }

  // Initialize inputStates to false
  for (int i = 0; i < NO_OF_INPUTS; ++i) {
    inputStates[i] = false;
    inputStatesLast[i] = false;
  }
}

void lcdInitialize();
void updateButtonInputStates();
void updateIRInputStates();
void runBusinessLogic();
void display();

void loop() {

  lcdInitialize();
  
  // Handle button debouncing and updating input states
  for (int i = 0; i < 4; i++) {
    byte currentButtonState = digitalRead(pushButtonPins[i]);

    if (currentButtonState != lastButtonState[i]) {
      // Check debounce timing
      if (millis() - lastTimeButtonStateChanged[i] >= debounceDuration) {
        lastTimeButtonStateChanged[i] = millis();  // Update debounce timer
        lastButtonState[i] = currentButtonState;   // Update the last state

        updateButtonInputStates();  // Call function to update button states
      }
    }
  }


  // Handle IR input updates
  updateIRInputStates();

  bool isInputChanged = false;
  for (int i = 0; i < NO_OF_INPUTS; ++i) {
    isInputChanged = isInputChanged || (inputStatesLast[i] != inputStates[i]);
    inputStatesLast[i] = inputStates[i];
  }
  if (isInputChanged) {
    for (int i = 0; i < NO_OF_INPUTS; ++i) {
      Serial.print(inputStates[i]);
    }
    Serial.println();
    runBusinessLogic();
    display();
  }
}

void lcdInitialize() {
    // Initialize the LCD
      lcd.setCursor(0, 0);  
        lcd.write(SOUND_ICON); 
      lcd.setCursor(4, 0); 
        lcd.write(TONE_ICON);
      lcd.setCursor(19, 0); 
        lcd.write(BATTERY_ICON);
}

void runBusinessLogic() {
// button abuwama variable wenas wena tika
}

void display() {
     // Display the sound level
    soundLevel[0] = MUTE;
    soundLevel[1] = LOW;
    soundLevel[2] = MEDIUM;
    soundLevel[3] = HIGH;
    switch (soundLevel) {
      case MUTE:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_MUTE_ICON);
        break;
      case LOW:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_LOW_ICON);
        break;
      case MEDIUM:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_MEDIUM_ICON);
        break;
      case HIGH:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_HIGH_ICON);
        break;
    }
    switch (soundLevel) {
          case 0xFFA25D:
            Serial.println("CH-");
            break;
          case 0xFF629D:
            Serial.println("CH");
            break;
}

void updateButtonInputStates() {
  // Update states for each button
  inputStates[BTN_UP] = digitalRead(pushButtonPins[BTN_UP]) == HIGH;
  inputStates[BTN_DOWN] = digitalRead(pushButtonPins[BTN_DOWN]) == HIGH;
  inputStates[BTN_START_STOP] = digitalRead(pushButtonPins[BTN_START_STOP]) == HIGH;
  inputStates[BTN_SET_RESET] = digitalRead(pushButtonPins[BTN_SET_RESET]) == HIGH;
}

void updateIRInputStates() {
  if (IrReceiver.decode()) {
    int resultCommand = IrReceiver.decodedIRData.command;

    // Update input states based on the received IR command
    inputStates[IR_CH_MIN] = (resultCommand == 0x45);
    inputStates[IR_CH] = (resultCommand == 0x46);
    inputStates[IR_CH_MAX] = (resultCommand == 0x47);
    inputStates[IR_LEFT_SHIFT] = (resultCommand == 0x44);
    inputStates[IR_RIGHT_SHIFT] = (resultCommand == 0x40);
    inputStates[IR_START_STOP] = (resultCommand == 0x43);
    inputStates[IR_MIN] = (resultCommand == 0x07);
    inputStates[IR_MAX] = (resultCommand == 0x15);
    inputStates[IR_EQ] = (resultCommand == 0x09);
    inputStates[IR_100PLUS] = (resultCommand == 0x19);
    inputStates[IR_200PLUS] = (resultCommand == 0x0D);
    inputStates[IR_0] = (resultCommand == 0x16);
    inputStates[IR_1] = (resultCommand == 0x0C);
    inputStates[IR_2] = (resultCommand == 0x18);
    inputStates[IR_3] = (resultCommand == 0x5E);
    inputStates[IR_4] = (resultCommand == 0x08);
    inputStates[IR_5] = (resultCommand == 0x1C);
    inputStates[IR_6] = (resultCommand == 0x5A);
    inputStates[IR_7] = (resultCommand == 0x42);
    inputStates[IR_8] = (resultCommand == 0x52);
    inputStates[IR_9] = (resultCommand == 0x4A);

    // Resume receiving the next IR command
    IrReceiver.resume();
  } else {
    for (int i = IR_CH_MIN; i <= IR_9; ++i) {
      inputStates[i] = false;
    }
  }
}