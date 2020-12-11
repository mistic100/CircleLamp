#include "FastLED.h"

#define LED_PIN 2
#define NUM_LEDS 60
#define LED_TYPE WS2812
#define COLOR_ORDER BRG
// R = Amber
// G = Warm White
// B = Cold White

#define BUTTON_PIN A6
#define BUTTON_THRES 500
#define BUTTON_DELAY 300
#define BUTTON_SUSTAIN_DELAY 1000
#define BUTTON_SUSTAIN_INTERVAL 40

#define TEMP_MAX 171
#define SPEED_MAX 50
#define SPEED_MIN 10

#define BTN_IDLE 0
#define BTN_SINGLE 1
#define BTN_SUSTAIN 2
#define BTN_RELEASE 3

#define MODE_STATIC 0
#define MODE_RAINBOW 1
#define MODE_RAINBOW_MOVE 2
#define MODE_MAX_ MODE_RAINBOW_MOVE

unsigned long btnTime = 0;
uint8_t btnState = BTN_IDLE;
bool btnDouble = false;

CRGB leds[NUM_LEDS];
uint8_t mode = MODE_STATIC;
bool on = true;
bool inConfig = false;
bool configDirection = true;
uint8_t paletteIndex = 0;
uint8_t brightness = 64;
uint8_t temperature = 128;
uint8_t speed = SPEED_MIN;

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
}

void loop() {
  manageButton();

  if (!on) {
    modeOff();
  }
  else {
    switch (mode) {
      case MODE_STATIC:
        modeStatic();
        break;

      case MODE_RAINBOW:
        modeRainbow(false);
        break;

      case MODE_RAINBOW_MOVE:
        modeRainbow(true);
        break;
    }
  }

  FastLED.setBrightness(brightness);
  FastLED.show();
  
  delay(20);
}

void modeOff() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void modeStatic() {
  CRGB color = ColorFromPalette(RainbowColors_p, temperature);
  fill_solid(leds, NUM_LEDS, color);
}

void modeRainbow(bool move) {
  if (move) {
    paletteIndex+= speed / 10.0;
  } else {
    paletteIndex = 0;
  }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t index = 255.0 * i / (NUM_LEDS - 1);
    leds[i] = ColorFromPalette(RainbowColors_p, index + paletteIndex);
  }
}

void blink(uint8_t nb) {
  for (uint8_t i = 0; i < nb; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.delay(200);
    fill_solid(leds, NUM_LEDS, CRGB::Gray);
    FastLED.delay(100);
  }
}

void singlePress() {
  on = !on;
  inConfig = false;
}

void doublePress() {
  if (!on) {
    return;
  }

  configDirection = true;
  
  if (mode == MODE_STATIC || mode == MODE_RAINBOW_MOVE) {
    if (inConfig) {
      inConfig = false;
      blink(2);
    } else {
      inConfig = true;
      blink(3);
    }
  }
}

void longPress() {
  if (!on || inConfig) {
    return;
  }
  
  mode++;
  if (mode > MODE_MAX_) {
    mode = 0;
  }
}

void sustain(bool isLast) {
  if (!on) {
    return;
  }

  if (isLast) {
    configDirection = !configDirection;
    return;
  }
  
  if (inConfig) {
    switch (mode) {
      case MODE_STATIC:
        if (configDirection) {
          if (temperature < TEMP_MAX) {
            temperature++;
          }
        } else {
          temperature = qsub8(temperature, 1);
        }
        break;
        
      case MODE_RAINBOW_MOVE:
        if (configDirection) {
          if (speed < SPEED_MAX) {
            speed++;
          }
        } else if (speed > SPEED_MIN) {
          speed--;
        }
        break;
    }

    return;
  }
  
  if (configDirection) {
    brightness = qadd8(brightness, 5);
  } else {
    brightness = qsub8(brightness, 5);
  }
}

void manageButton() {
  if (analogRead(BUTTON_PIN) > BUTTON_THRES) {
    switch (btnState) {
      case BTN_RELEASE:
        btnDouble = true;
      case BTN_IDLE:
        btnTime = millis();
        btnState = BTN_SINGLE;
        break;
        
      case BTN_SINGLE:
        if (millis() - btnTime > BUTTON_SUSTAIN_DELAY) {
          sustain(false);
          btnTime = millis();
          btnState = BTN_SUSTAIN;
        }
        break;
        
      case BTN_SUSTAIN:
        if (millis() - btnTime > BUTTON_SUSTAIN_INTERVAL) {
          sustain(false);
          btnTime = millis();
        }
        break;
    }
    
  } else {
    switch (btnState) {
      case BTN_SINGLE:
        if (millis() - btnTime < BUTTON_DELAY) {
          btnTime = millis();
          btnState = BTN_RELEASE;
        } else {
          longPress();
          btnState = BTN_IDLE;
          btnDouble = false;
        }
        break;
        
      case BTN_SUSTAIN:
        sustain(true);
        btnState = BTN_IDLE;
        btnDouble = false;
        break;

      case BTN_RELEASE:
        if (btnDouble) {
          doublePress();
          btnState = BTN_IDLE;
          btnDouble = false;
        } else if (millis() - btnTime >= BUTTON_DELAY) {
          singlePress();
          btnState = BTN_IDLE;
          btnDouble = false;
        }
        break;
    }
  }
}
