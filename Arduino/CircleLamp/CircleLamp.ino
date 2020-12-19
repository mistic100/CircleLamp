#include "FastLED.h"

#define LED_PIN 4
#define NUM_LEDS 60
#define LED_TYPE WS2812
#define COLOR_ORDER BRG
// R = Amber
// G = Warm White
// B = Cold White

#define BUTTON_PIN A0
#define BUTTON_THRES 500
#define BUTTON_DELAY 300
#define BUTTON_SUSTAIN_DELAY 1000
#define BUTTON_SUSTAIN_INTERVAL 40

#define TEMP_MAX 171

#define BTN_IDLE 0
#define BTN_SINGLE 1
#define BTN_SUSTAIN 2
#define BTN_RELEASE 3

#define MODE_FULL 0
#define MODE_STATIC 1
#define MODE_RAINBOW 2
#define MODE_CANDLE 3
#define MODE_MAX_ MODE_CANDLE

unsigned long btnTime = 0;
uint8_t btnState = BTN_IDLE;
bool btnDouble = false;

CRGB leds[NUM_LEDS];
uint8_t mode = MODE_FULL;
bool on = true;
bool inConfig = false;
bool configDirection = true;
uint8_t brightness = 128;
uint8_t temperature = 128;

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
      case MODE_FULL:
        modeFull();
        break;
      
      case MODE_STATIC:
        modeStatic();
        break;

      case MODE_RAINBOW:
        modeRainbow();
        break;

      case MODE_CANDLE:
        modeCandle();
        break;
    }
  }

  FastLED.setBrightness(brightness);
  FastLED.show();
  
  FastLED.delay(20);
}

void modeOff() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void modeFull() {
  fill_solid(leds, NUM_LEDS, CRGB::White);
}

void modeStatic() {
  CRGB color = ColorFromPalette(RainbowColors_p, temperature);
  fill_solid(leds, NUM_LEDS, color);
}

void modeRainbow() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t index = 255.0 * i / (NUM_LEDS - 1);
    leds[i] = ColorFromPalette(RainbowColors_p, index);
  }
}

#define HALF_LEDS NUM_LEDS / 2
void modeCandle() {
  // Array of temperature readings at each simulation cell
  static byte heat[HALF_LEDS];

  static uint8_t IGNIT = 8;
  static uint8_t COOLING = 40;
  static uint8_t SPARKING = 120;

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < HALF_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / HALF_LEDS) + 2));
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = HALF_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 1] + heat[k - 2] + heat[k - 3]) / 5;
  }
  
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING) {
    int y = random8(IGNIT);
    heat[y] = qadd8(heat[y], random8(160-y,255-y));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < HALF_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8(heat[j], 240);
    CRGB color = ColorFromPalette(HeatColors_p, colorindex);
    
    leds[j] = color;
    leds[NUM_LEDS - 1 - j] = color;
  }

  FastLED.delay(10);
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
  
  if (mode == MODE_STATIC) {
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
