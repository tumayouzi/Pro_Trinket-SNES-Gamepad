/*
 SNES Gamepad Control for Pro Triket

 Controls a PC gamepad from a joystick on an Adafruit Pro Trinket.

 This code was inspired by zfergus / trinket-snes-controller.
 URL: https://github.com/zfergus/trinket-snes-controller

 The gamepad library uses buxit / Pro_Trinket_USB_Gamepad.
 URL: https://github.com/buxit/Pro_Trinket_USB_Gamepad

 Buttons are recognized in the order of A, B, X, Y, L, R, select and start.
 Cross buttons are recognized as X and Y axes.

 WARNING:  When you use the new gamepad, the Arduino takes
 over your gamepad!  Make sure you have control before you use the project.

Based on software on arduino.cc by Tom Igoe placed in the public domain

 */
#include <ProTrinketGamepad.h>  // include gamepad library for Pro Trinket (3V or 5V)
#include <MsTimer2.h>

/* for SNES controller */
#define LATCH_PIN 3
#define DATA_PIN 6
#define CLOCK_PIN 5

/* optional setting */
#define BUTTON_FUNCTION 10
#define LED_FUNCTION 9
#define LED_BOARD 13 //Boad LED for heartbeat
#define LED_INPUT 12 //Input status LED

//Function Key Enable count
#define BTN_COUNT_SHORT 5

/* for SNES controller pin setting */
#define SET_LATCH(state) digitalWrite(LATCH_PIN, state)
#define SET_CLOCK(state) digitalWrite(CLOCK_PIN, state)
#define GET_DATA() digitalRead(DATA_PIN)

#define WAIT(microsec) delayMicroseconds(microsec)

int btnCount = 0;
int btnStatus = 0;
int btnEnableShort = 0;

uint16_t buttonMap;
int16_t axis[4] = {0, 0, 0, 0}; //x, y, x roll, y roll

unsigned short pressedButtons;

const int responseDelay = 5; // response delay of the gamepad, in ms

const int16_t AXIS_LEVEL_MAX = 32767;
const int16_t AXIS_LEVEL_MIN = -32768;
const int16_t AXIS_LEVEL_NEUTRAL = 0;

void setup() {
  /* Set pin mode for SNES controller */
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode( DATA_PIN,  INPUT);

  /* Set pin mode for optional */
  pinMode(LED_FUNCTION, OUTPUT);
  pinMode(BUTTON_FUNCTION, INPUT_PULLUP);
  pinMode(LED_BOARD, OUTPUT);
  pinMode(LED_INPUT, OUTPUT);

  /*Set pin state */
  digitalWrite(LED_FUNCTION,LOW);
  digitalWrite(LED_INPUT,HIGH);

  pressedButtons = 0;

  /* for heartbeat */
  MsTimer2::set(500, timerFire);
  MsTimer2::start();

  TrinketGamepad.begin();
}

void loop() {
  if(digitalRead(BUTTON_FUNCTION) == 0) {
    btnCount++;
  } else if(digitalRead(BUTTON_FUNCTION) == 1 ) {
    if(btnCount >= BTN_COUNT_SHORT){
      btnEnableShort = 1;
      btnCount = 0;
    } else {
      btnCount = 0;
    }
  }

  if(btnEnableShort == 1){
    if(btnStatus == 0) {
      btnStatus = 1;
      digitalWrite(LED_FUNCTION,HIGH);
    } else if(btnStatus == 1) {
      btnStatus = 0;
      digitalWrite(LED_FUNCTION,LOW);
    }
    btnEnableShort = 0;
    }

  unsigned short data = getSNESData();

  byte i;

  for(i = 0; i < 12; i++) {
    if((data >> i) & 0x01 && !((pressedButtons >> i) & 0x1)) {
      if (i == 0) { //B
        bitWrite(buttonMap, i + 1, 1);
      } else if (i >= 1 && i <= 3) { //Y, Select, Start
        bitWrite(buttonMap, i | (i * 2), 1);
      } else if (i >= 8 && i <= 10) { //A, X, L
        bitWrite(buttonMap, (i * 2) & 15, 1);
      } else if (i == 11) { //R
        bitWrite(buttonMap, ((i * 2) & 15) - 1, 1);
      } else if (i == 4 || i == 6) { //Up, Left
        axis[!((i >> 1) & 1)] = AXIS_LEVEL_MIN;
      } else if (i == 5 || i == 7) { //Down, Right
        axis[!((i >> 1) & 1)] = AXIS_LEVEL_MAX;
      }

      pressedButtons |= 0x01 << i;
      digitalWrite(LED_INPUT,LOW);
    }
    else if(!((data >> i) & 0x01) && (pressedButtons >> i) & 0x1) {
      if (i == 0) {
        bitWrite(buttonMap, i + 1, 0);
      } else if (i >= 1 && i <= 3) {
        bitWrite(buttonMap, i | (i * 2), 0);
      } else if (i >= 8 && i <= 10) {
        bitWrite(buttonMap, (i * 2) & 15, 0);
      } else if (i == 11) {
        bitWrite(buttonMap, ((i * 2) & 15) - 1, 0);
      } else if (i == 4 || i == 6) {
        axis[!((i >> 1) & 1)] = AXIS_LEVEL_NEUTRAL;
      } else if (i == 5 || i == 7) {
        axis[!((i >> 1) & 1)] = AXIS_LEVEL_NEUTRAL;
      }

      pressedButtons &= ~(0x01 << i);
      digitalWrite(LED_INPUT,HIGH);
    }
  }

  TrinketGamepad.move(axis[0], //x
                      axis[1], //y
                      axis[2], //x Roll
                      axis[3], //y Roll
                      buttonMap
                      );

  delay(responseDelay);  // wait between gamepad readings
}

unsigned short getSNESData() {
  unsigned short data = 0;

  /* Tell the SNES controller to send data. */
  SET_LATCH(LOW);
  SET_CLOCK(LOW);

  SET_LATCH(HIGH);
  WAIT(12);
  SET_LATCH(LOW);

  byte i;
  for (i = 0; i < 16; i++) {
    WAIT(6);
    SET_CLOCK(LOW);

    /* Save if button i is being pressed, 1, or not, 0. */
    data |= ((GET_DATA() == LOW ? 1:0) << i);

    WAIT(6);
    SET_CLOCK(HIGH);
  }

  return data;
}

//heatbeat
void timerFire() {
  digitalWrite(LED_BOARD, !digitalRead(LED_BOARD));
}
