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

enum DisplayMode {
  DEFAULT_MODE,
  BLINK_SECOND_MODE,
  BLINK_MINUTE_MODE,
  BLINK_HOUR_MODE,
  BLINK_SOUND_MODE,
  BLINK_TONE_MODE,
  TIME_SET_MODE,
  RUNNING_MODE,
  PAUSE_MODE
};

enum CustomChar {
  SOUND_ICON,
  SOUND_S_MUTE_ICON,
  SOUND_S_LOW_ICON,
  SOUND_S_MEDIUM_ICON,
  SOUND_S_HIGH_ICON,
  TONE_ICON,
  BATTERY_ICON,
  UNDERLINE_CHARACTER,
  CLEAR_CHARACTER
};

enum Sound {
  S_MUTE,
  S_LOW,
  S_MEDIUM,
  S_HIGH
};

enum Tone {
  TONE_1 = 1,
  TONE_2 = 2,
  TONE_3 = 3
};

enum HoverPosition {
  H_NONE,
  H_SECONDS,
  H_MINUTES,
  H_HOURS,
  H_SOUND,
  H_TONE
};

unsigned long targetMillis = 0;
unsigned long currentDisplayMillis = 0;

unsigned long startMillis = 0;
unsigned long pauseMillis = 0;

int currentHoverPosition = H_NONE;

int soundLevel = S_MEDIUM;
int toneNumber = TONE_1;
int displayMode = DEFAULT_MODE;

const int pushButtonPins[] = { 3, 4, 5, 6 };

bool inputStates[NO_OF_INPUTS];
bool inputStatesLast[NO_OF_INPUTS];

int noCount[3] = {0};

byte lastButtonState[4];
unsigned long lastTimeButtonStateChanged[4] = { 0 };
unsigned long debounceDuration = 200;  // millis

int buzzer = 9;
LiquidCrystal_I2C lcd(0x27, 20, 4);

byte underlineCharacter[] = {
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte clearCharacter[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

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

byte soundS_MUTEIcon[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte soundS_LOWIcon[] = {
  B00000,
  B00000,
  B00000,
  B10000,
  B10000,
  B00000,
  B00000,
  B00000
};

byte soundS_MEDIUMIcon[] = {
  B00000,
  B01000,
  B00100,
  B10100,
  B10100,
  B00100,
  B01000,
  B00000
};

byte soundS_HIGHIcon[] = {
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

void lcdInitialize();
void lcdDefaultValueInitialize();

void updateButtonInputStates();
void updateIRInputStates();
void runInputLogic();
void display();

//Busniss logic funtion
void secondTimeIncrease();
void minuteTimeIncrease();
void hourTimeIncrease();
void soundIncrease();
void toneIncrease();

void secondTimeDecrease();
void minuteTimeDecrease();
void hourTimeDecrease();
void soundDecrease();
void toneDecrease();

void changeInToBlinkSecondMode();
void changeInToBlinkMinuteMode();
void changeInToBlinkHourMode();
void changeInToBlinkSoundMode();
void changeInToBlinkToneMode();

void changeInToDefaultMode();
void changeInToPauseMode();
void changeInToRunningMode();
void changeInToSetMode();

void quick5min();
void quick10min();
void quick20min();
void quick30min();
void quick1hour();
void quick1hour30min();
void quick2hour();
void quick2hour30min();
void quick3hour();
void quick4hour();

void printNumber();

void updateCurrentCountDownTime();

void resetNoCount();

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;  // Wait for Serial to become available.

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  lcd.init();
  lcd.backlight();
  lcd.createChar(SOUND_ICON, soundIcon);
  lcd.createChar(SOUND_S_MUTE_ICON, soundS_MUTEIcon);
  lcd.createChar(SOUND_S_LOW_ICON, soundS_LOWIcon);
  lcd.createChar(SOUND_S_MEDIUM_ICON, soundS_MEDIUMIcon);
  lcd.createChar(SOUND_S_HIGH_ICON, soundS_HIGHIcon);
  lcd.createChar(TONE_ICON, toneIcon);
  lcd.createChar(BATTERY_ICON, batteryIcon);
  lcd.createChar(UNDERLINE_CHARACTER, underlineCharacter);
  lcd.createChar(CLEAR_CHARACTER, clearCharacter);
  lcd.home();

  lcdInitialize();
  lcdDefaultValueInitialize();

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

void loop() {

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
    runInputLogic();
  }

  if (displayMode == RUNNING_MODE) {
    updateCurrentCountDownTime();
  }

  display();
  lcdInitialize();
}

void updateCurrentCountDownTime() {
    unsigned long currentMillis = millis(); // Get the current time in milliseconds
    unsigned long elapsedMillis = currentMillis - startMillis;  // Calculate elapsed time

    // Check if the countdown is complete
    if (elapsedMillis >= targetMillis) {
        // Countdown finished, remaining time is 0
        currentDisplayMillis = 0;
        displayMode = DEFAULT_MODE;
        makeSound();
    } else {
        // Calculate the remaining time in milliseconds
        currentDisplayMillis = targetMillis - elapsedMillis;
    }
}

void makeSound() {
    int freq = 450;

    switch(soundLevel) {
    case S_MUTE:
        return;
        break;
    case S_LOW:
        freq = 200;
        break;
    case S_MEDIUM:
        freq = 300;
        break;
    case S_HIGH:
        freq = 500;
        break;
    }

    switch(toneNumber) {
    case TONE_1:
       tone(buzzer, freq);
       delay (500);
       noTone(buzzer);
       delay(500);
    case TONE_2:
       tone(buzzer, freq);
       delay (1000);
       noTone(buzzer);
       delay(1000);
    case TONE_3:
       tone(buzzer, freq);
       delay (2000);
       noTone(buzzer);
       delay(2000);
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

void lcdDefaultValueInitialize() {
  // Display DEFAULT value
  lcd.setCursor(1, 0);
  lcd.write(SOUND_S_MEDIUM_ICON);
  lcd.setCursor(5, 0);
  lcd.print(1);
  lcd.setCursor(6, 2);
  lcd.print("00:00:00");
}

void runInputLogic() {

  for (int i = BTN_UP; i <= IR_9; ++i) {
    if (!inputStates[i]) continue;

    switch (i) {
      case BTN_UP:
        switch (displayMode) {
          case DEFAULT_MODE:
            // Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            secondTimeIncrease();
            break;
          case BLINK_MINUTE_MODE:
            minuteTimeIncrease();
            break;
          case BLINK_HOUR_MODE:
            hourTimeIncrease();
            break;
          case BLINK_SOUND_MODE:
            soundIncrease();
            break;
          case BLINK_TONE_MODE:
            toneIncrease();
            break;
          case TIME_SET_MODE:
            // Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            // Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            // Do nothing in PAUSE_MODE
            break;
        }
        break;
      case BTN_DOWN:
        switch (displayMode) {
          case DEFAULT_MODE:
            // Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            secondTimeDecrease();
            break;
          case BLINK_MINUTE_MODE:
            minuteTimeDecrease();
            break;
          case BLINK_HOUR_MODE:
            hourTimeDecrease();
            break;
          case BLINK_SOUND_MODE:
            soundDecrease();
            break;
          case BLINK_TONE_MODE:
            toneDecrease();
            break;
          case TIME_SET_MODE:
            // Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            // Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            // Do nothing in PAUSE_MODE
            break;
        }
        break;
      case BTN_START_STOP:
        switch (displayMode) {
          case DEFAULT_MODE:
            // Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            changeInToSetMode();
            break;
          case BLINK_MINUTE_MODE:
            changeInToSetMode();
            break;
          case BLINK_HOUR_MODE:
            changeInToSetMode();
            break;
          case BLINK_SOUND_MODE:
            changeInToSetMode();
            break;
          case BLINK_TONE_MODE:
            changeInToSetMode();
            break;
          case TIME_SET_MODE:
            changeInToRunningMode();
            break;
          case RUNNING_MODE:
            changeInToPauseMode();
            break;
          case PAUSE_MODE:
            changeInToRunningMode();
            break;
        }
        break;
      case BTN_SET_RESET:
        resetNoCount();
        switch (displayMode) {
          case DEFAULT_MODE:
            changeInToBlinkSecondMode();
            break;
          case BLINK_SECOND_MODE:
            changeInToBlinkMinuteMode();
            break;
          case BLINK_MINUTE_MODE:
            changeInToBlinkHourMode();
            break;
          case BLINK_HOUR_MODE:
            changeInToBlinkSoundMode();
            break;
          case BLINK_SOUND_MODE:
            changeInToBlinkToneMode();
            break;
          case BLINK_TONE_MODE:
            changeInToSetMode();
            break;
          case TIME_SET_MODE:
            changeInToDefaultMode();
            break;
          case RUNNING_MODE:
            changeInToDefaultMode();
            break;
          case PAUSE_MODE:
            changeInToDefaultMode();
            break;
        }
        break;
      case IR_CH_MIN:
        resetNoCount();
        switch (displayMode) {
          case DEFAULT_MODE:
            //Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            changeInToBlinkSoundMode();
            break;
          case BLINK_MINUTE_MODE:
            changeInToBlinkSoundMode();
            break;
          case BLINK_HOUR_MODE:
            changeInToBlinkSoundMode();
            break;
          case BLINK_SOUND_MODE:
            changeInToBlinkToneMode();
            break;
          case BLINK_TONE_MODE:
            changeInToBlinkSecondMode();
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_CH:
        resetNoCount();
        switch (displayMode) {
          case DEFAULT_MODE:
            changeInToBlinkSecondMode();
            break;
          case BLINK_SECOND_MODE:
            changeInToSetMode();
            break;
          case BLINK_MINUTE_MODE:
            changeInToSetMode();
            break;
          case BLINK_HOUR_MODE:
            changeInToSetMode();
            break;
          case BLINK_SOUND_MODE:
            changeInToSetMode();
            break;
          case BLINK_TONE_MODE:
            changeInToSetMode();
            break;
          case TIME_SET_MODE:
            changeInToDefaultMode();
            break;
          case RUNNING_MODE:
            changeInToDefaultMode();
            break;
          case PAUSE_MODE:
            changeInToDefaultMode();
            break;
        }
        break;
      case IR_CH_MAX:
        resetNoCount();
        switch (displayMode) {
          case DEFAULT_MODE:
            //Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            changeInToBlinkSoundMode();
            break;
          case BLINK_MINUTE_MODE:
            changeInToBlinkSoundMode();
            break;
          case BLINK_HOUR_MODE:
            changeInToBlinkSoundMode();
            break;
          case BLINK_SOUND_MODE:
            changeInToBlinkToneMode();
            break;
          case BLINK_TONE_MODE:
            changeInToBlinkSecondMode();
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_LEFT_SHIFT:
        resetNoCount();
        switch (displayMode) {
          case DEFAULT_MODE:
            //Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            changeInToBlinkMinuteMode();
            break;
          case BLINK_MINUTE_MODE:
            changeInToBlinkHourMode();
            break;
          case BLINK_HOUR_MODE:
            changeInToBlinkSecondMode();
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_RIGHT_SHIFT:
        resetNoCount();
        switch (displayMode) {
          case DEFAULT_MODE:
            //Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            changeInToBlinkHourMode();
            break;
          case BLINK_MINUTE_MODE:
            changeInToBlinkSecondMode();
            break;
          case BLINK_HOUR_MODE:
            changeInToBlinkMinuteMode();
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_START_STOP:
        switch (displayMode) {
          case DEFAULT_MODE:
            //Do nothing in DEFAULT_MODE
            break;
          case BLINK_SECOND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_MINUTE_MODE:
            //Do nothing in BLINK_MINUTE_MODE
            break;
          case BLINK_HOUR_MODE:
            //Do nothing in BLINK_HOUR_MODE
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            changeInToPauseMode();
            break;
          case PAUSE_MODE:
            changeInToRunningMode();
            break;
        }
        break;
      case IR_MIN:
        switch (displayMode) {
          case DEFAULT_MODE:
            soundDecrease();
            break;
          case BLINK_SECOND_MODE:
            secondTimeDecrease();
            break;
          case BLINK_MINUTE_MODE:
            minuteTimeDecrease();
            break;
          case BLINK_HOUR_MODE:
            hourTimeDecrease();
            break;
          case BLINK_SOUND_MODE:
            soundDecrease();
            break;
          case BLINK_TONE_MODE:
            toneDecrease();
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_MAX:
        switch (displayMode) {
          case DEFAULT_MODE:
            soundIncrease();
            break;
          case BLINK_SECOND_MODE:
            secondTimeIncrease();
            break;
          case BLINK_MINUTE_MODE:
            minuteTimeIncrease();
            break;
          case BLINK_HOUR_MODE:
            hourTimeIncrease();
            break;
          case BLINK_SOUND_MODE:
            soundIncrease();
            break;
          case BLINK_TONE_MODE:
            toneIncrease();
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_EQ:
        //Do nothing
        break;
      case IR_100PLUS:
        //Do nothing
        break;
      case IR_200PLUS:
        //Do nothing
        break;
      case IR_0:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick5min();
            break;
          case BLINK_SECOND_MODE:
            printNumber(0);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(0);
            break;
          case BLINK_HOUR_MODE:
            printNumber(0);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_1:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick10min();
            break;
          case BLINK_SECOND_MODE:
            printNumber(1);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(1);
            break;
          case BLINK_HOUR_MODE:
            printNumber(1);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_2:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick20min();
            break;
          case BLINK_SECOND_MODE:
            printNumber(2);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(2);
            break;
          case BLINK_HOUR_MODE:
            printNumber(2);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_3:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick30min();
            break;
          case BLINK_SECOND_MODE:
            printNumber(3);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(3);
            break;
          case BLINK_HOUR_MODE:
            printNumber(3);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_4:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick1hour();
            break;
          case BLINK_SECOND_MODE:
            printNumber(4);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(4);
            break;
          case BLINK_HOUR_MODE:
            printNumber(4);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_5:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick1hour30min();
            break;
          case BLINK_SECOND_MODE:
            printNumber(5);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(5);
            break;
          case BLINK_HOUR_MODE:
            printNumber(5);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_6:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick2hour();
            break;
          case BLINK_SECOND_MODE:
            printNumber(6);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(6);
            break;
          case BLINK_HOUR_MODE:
            printNumber(6);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_7:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick2hour30min();
            break;
          case BLINK_SECOND_MODE:
            printNumber(7);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(7);
            break;
          case BLINK_HOUR_MODE:
            printNumber(7);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_8:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick3hour();
            break;
          case BLINK_SECOND_MODE:
            printNumber(8);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(8);
            break;
          case BLINK_HOUR_MODE:
            printNumber(8);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
      case IR_9:
        switch (displayMode) {
          case DEFAULT_MODE:
            quick4hour();
            break;
          case BLINK_SECOND_MODE:
            printNumber(9);
            break;
          case BLINK_MINUTE_MODE:
            printNumber(9);
            break;
          case BLINK_HOUR_MODE:
            printNumber(9);
            break;
          case BLINK_SOUND_MODE:
            //Do nothing in BLINK_SOUND_MODE
            break;
          case BLINK_TONE_MODE:
            //Do nothing in BLINK_TONE_MODE
            break;
          case TIME_SET_MODE:
            //Do nothing in TIME_SET_MODE
            break;
          case RUNNING_MODE:
            //Do nothing in RUNNING_MODE
            break;
          case PAUSE_MODE:
            //Do nothing in PAUSE_MODE
            break;
        }
        break;
    }
  }
}

void display() {

    switch (soundLevel) {
      case S_MUTE:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_S_MUTE_ICON);
        break;
      case S_LOW:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_S_LOW_ICON);
        break;
      case S_MEDIUM:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_S_MEDIUM_ICON);
        break;
      case S_HIGH:
        lcd.setCursor(1, 0);
        lcd.write(SOUND_S_HIGH_ICON);
        break;
    }

    switch (toneNumber) {
      case TONE_1:
        lcd.setCursor(5, 0);
        lcd.print(1);
        break;
      case TONE_2:
        lcd.setCursor(5, 0);
        lcd.print(2);
        break;
      case TONE_3:
        lcd.setCursor(5, 0);
        lcd.print(3);
        break;
    }

    // Convert milliseconds to hours, minutes, and seconds
    unsigned long totalSeconds = currentDisplayMillis / 1000;
    int seconds = totalSeconds % 60;
    int minutes = (totalSeconds / 60) % 60;
    int hours = (totalSeconds / 3600) % 24;  // Assuming 24-hour format

    // Print hours
    lcd.setCursor(6, 2);
    if (hours < 10) lcd.print('0');
    lcd.print(hours);
    lcd.print(':');

    // Print minutes
    lcd.setCursor(9, 2);
    if (minutes < 10) lcd.print('0');
    lcd.print(minutes);
    lcd.print(':');

    // Print seconds
    lcd.setCursor(12, 2);
    if (seconds < 10) lcd.print('0');
    lcd.print(seconds);

    switch (currentHoverPosition) {
      case H_NONE:
        lcd.setCursor(12, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(13, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(9, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(10, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(6, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(7, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(1, 1);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(5, 1);
        lcd.write(CLEAR_CHARACTER);
        break;
      case H_SECONDS:
        lcd.setCursor(12, 3);
        lcd.write(UNDERLINE_CHARACTER);
        lcd.setCursor(13, 3);
        lcd.write(UNDERLINE_CHARACTER);
        lcd.setCursor(9, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(10, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(6, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(7, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(1, 1);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(5, 1);
        lcd.write(CLEAR_CHARACTER);
        break;
      case H_MINUTES:
        lcd.setCursor(12, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(13, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(9, 3);
        lcd.write(UNDERLINE_CHARACTER);
        lcd.setCursor(10, 3);
        lcd.write(UNDERLINE_CHARACTER);
        lcd.setCursor(6, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(7, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(1, 1);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(5, 1);
        lcd.write(CLEAR_CHARACTER);
        break;
      case H_HOURS:
        lcd.setCursor(12, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(13, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(9, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(10, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(6, 3);
        lcd.write(UNDERLINE_CHARACTER);
        lcd.setCursor(7, 3);
        lcd.write(UNDERLINE_CHARACTER);
        lcd.setCursor(1, 1);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(5, 1);
        lcd.write(CLEAR_CHARACTER);
        break;
      case H_SOUND:
        lcd.setCursor(12, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(13, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(9, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(10, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(6, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(7, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(1, 1);
        lcd.write(UNDERLINE_CHARACTER);
        lcd.setCursor(5, 1);
        lcd.write(CLEAR_CHARACTER);
        break;
      case H_TONE:
        lcd.setCursor(12, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(13, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(9, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(10, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(6, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(7, 3);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(1, 1);
        lcd.write(CLEAR_CHARACTER);
        lcd.setCursor(5, 1);
        lcd.write(UNDERLINE_CHARACTER);
        break;
    }
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

void secondTimeIncrease() {
    unsigned long additionalMillis = 1000;
    unsigned long newMillis = targetMillis + additionalMillis;
    if (newMillis > 359964000) {
        newMillis = 359964000;
    }
    targetMillis += additionalMillis;
    currentDisplayMillis = targetMillis;
}

void minuteTimeIncrease() {
    unsigned long additionalMillis = 60 * 1000;
    unsigned long newMillis = targetMillis + additionalMillis;
    if (newMillis > 359964000) {
        newMillis = 359964000;
    }
    targetMillis += additionalMillis;
    currentDisplayMillis = targetMillis;
}

void hourTimeIncrease() {
    unsigned long additionalMillis = 60 * 60 * 1000;
    unsigned long newMillis = targetMillis + additionalMillis;
    if (newMillis > 359964000) {
        newMillis = 359964000;
    }
    targetMillis += additionalMillis;
    currentDisplayMillis = targetMillis;
}

void soundIncrease() {
    int newSoundLevel = soundLevel + 1;
    if (newSoundLevel > S_HIGH) {
        newSoundLevel = S_HIGH;
    }
    soundLevel = newSoundLevel;
}

void toneIncrease() {
    int newToneNumber = toneNumber + 1;
    if (newToneNumber > TONE_3) {
        newToneNumber = TONE_3;
    }
    toneNumber = newToneNumber;
}

void secondTimeDecrease() {
    unsigned long additionalMillis = -1000;
    unsigned long newMillis = targetMillis + additionalMillis;
    if (newMillis < 0) {
        newMillis = 0;
    }
    targetMillis += additionalMillis;
    currentDisplayMillis = targetMillis;
}

void minuteTimeDecrease() {
    unsigned long additionalMillis = -60 * 1000;
    unsigned long newMillis = targetMillis + additionalMillis;
    if (newMillis < 0) {
        newMillis = 0;
    }
    targetMillis += additionalMillis;
    currentDisplayMillis = targetMillis;
}

void hourTimeDecrease() {
    unsigned long additionalMillis = -60 * 60 * 1000;
    unsigned long newMillis = targetMillis + additionalMillis;
    if (newMillis < 0) {
        newMillis = 0;
    }
    targetMillis += additionalMillis;
    currentDisplayMillis = targetMillis;
}

void soundDecrease() {
    int newSoundLevel = soundLevel - 1;
    if (newSoundLevel < S_MUTE) {
        newSoundLevel = S_MUTE;
    }
    soundLevel = newSoundLevel;
}

void toneDecrease() {
    int newToneNumber = toneNumber - 1;
    if (newToneNumber < TONE_1) {
        newToneNumber = TONE_1;
    }
    toneNumber = newToneNumber;
}

void changeInToBlinkSecondMode() {
    displayMode = BLINK_SECOND_MODE;
    currentHoverPosition = H_SECONDS;
}

void changeInToBlinkMinuteMode() {
    displayMode = BLINK_MINUTE_MODE;
    currentHoverPosition = H_MINUTES;
}

void changeInToBlinkHourMode() {
    displayMode = BLINK_HOUR_MODE;
    currentHoverPosition = H_HOURS;
}

void changeInToBlinkSoundMode() {
    displayMode = BLINK_SOUND_MODE;
    currentHoverPosition = H_SOUND;
}

void changeInToBlinkToneMode() {
    displayMode = BLINK_TONE_MODE;
    currentHoverPosition = H_TONE;
}

void changeInToDefaultMode() {
    displayMode = DEFAULT_MODE;
    currentHoverPosition = H_NONE;
}

void changeInToPauseMode() {
    displayMode = PAUSE_MODE;
    targetMillis = currentDisplayMillis;
    startMillis = 0;
}

void changeInToRunningMode() {
    displayMode = RUNNING_MODE;
    startMillis = millis();
    currentDisplayMillis = targetMillis;
}

void changeInToSetMode() {
    if (targetMillis == 0) {
        changeInToDefaultMode();
        return;
    }
    displayMode = TIME_SET_MODE;
    currentHoverPosition = H_NONE;
}

void quick5min() {
    targetMillis = 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick10min() {
    targetMillis = 2 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick20min() {
    targetMillis = 4 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick30min() {
    targetMillis = 6 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick1hour() {
    targetMillis = 12 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick1hour30min() {
    targetMillis = 18 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick2hour() {
    targetMillis = 24 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick2hour30min() {
    targetMillis = 30 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick3hour() {
    targetMillis = 36 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void quick4hour() {
    targetMillis = 48 * 300000;
    currentDisplayMillis = targetMillis;
    changeInToSetMode();
}

void printNumber(int num) {

    // Convert milliseconds to hours, minutes, and seconds
    unsigned long totalSeconds = currentDisplayMillis / 1000;
    int seconds = totalSeconds % 60;
    int minutes = (totalSeconds / 60) % 60;
    int hours = (totalSeconds / 3600) % 24;  // Assuming 24-hour format

    switch(displayMode) {
    case BLINK_SECOND_MODE:
        if (noCount[H_SECONDS - 1] == 0) {
            seconds = num;
            noCount[H_SECONDS - 1]++;
        } else if (noCount[H_SECONDS - 1] == 1) {
            seconds += 10 * num;
            noCount[H_SECONDS - 1]++;
        } else {
            seconds = num;
            noCount[H_SECONDS - 1] = 1;
        }
        break;
    case BLINK_MINUTE_MODE:
        if (noCount[H_MINUTES - 1] == 0) {
            minutes = num;
            noCount[H_MINUTES - 1]++;
        } else if (noCount[H_MINUTES - 1] == 1) {
            minutes += 10 * num;
            noCount[H_MINUTES - 1]++;
        } else {
            minutes = num;
            noCount[H_MINUTES - 1] = 1;
        }
        break;
    case BLINK_HOUR_MODE:
        if (noCount[H_HOURS - 1] == 0) {
            hours = num;
            noCount[H_HOURS - 1]++;
        } else if (noCount[H_HOURS - 1] == 1) {
            hours += 10 * num;
            noCount[H_HOURS - 1]++;
        } else {
            hours = num;
            noCount[H_HOURS - 1] = 1;
        }
        break;
    }

    const unsigned long MILLIS_PER_SECOND = 1000;
    const unsigned long MILLIS_PER_MINUTE = 60 * MILLIS_PER_SECOND;
    const unsigned long MILLIS_PER_HOUR = 60 * MILLIS_PER_MINUTE;

    targetMillis = (hours * MILLIS_PER_HOUR) + (minutes * MILLIS_PER_MINUTE) + (seconds * MILLIS_PER_SECOND);
    currentDisplayMillis = targetMillis;
}

void resetNoCount() {
    noCount[0] = 0;
    noCount[1] = 1;
    noCount[2] = 2;
}
