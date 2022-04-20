/*
 * Basiert auf https://github.com/ancalex/Fiber-Optic-LED-Lamp
 * und wurde von mir angepasst auf ESP8266 (WEMOS D1 Mini)
 * Weiter habe ich die Funktion für Power-Off herausgenommen und den Single-Klick
 * auf den Long-Klick gesetzt. Grund:
 * In der HW werden Touch-Sensoren eingesetzt. ein Single-Klick-Funktion klappt
 * nicht, da die Sensoren minimal100ms Pulsbreite erzeugen. Für die Funktion
 * Single-Klick zu lang.  * .isSingleClick wurde entfernt.
 * 
 * Veränderungen von Betriebsart und Einstellungen werden
 * wie bisher im EEPROM gespeichert. Diese Routinen wurden auch auf
 * ESP8266 angepasst.
 */
#include <FastLED.h>
#include <PinButton.h>
#include <EEPROM.h>

// define the LEDs
#define LED_PIN D8  //pin the LEDs are connected to
#define NUM_LEDS 32
#define BRIGHTNESS 200 //maximum brightness
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
struct CRGB leds[NUM_LEDS];
#define UPDATES_PER_SECOND 100

#include "solid_color_mode.h"
#include "palette_mode.h"
#include "effect_mode.h"

const int wakeUpPin = D7;
PinButton FunctionButton(wakeUpPin);
int setModeLampe = 0;
int validEEPROM = 0;
bool firstRun = true;

// Konfigurations-Struct
struct VarData {
  int valid;
  int strusetModeLampe;
  int strucolorCounter;
  int strupaletteCounter;
  int strugCurrentPatternNumber;
};
struct VarData Config;

void initEEPROM(void) {
  Config.valid = 0x55aa;
  Config.strusetModeLampe = 0;
  Config.strucolorCounter = 0;
  Config.strupaletteCounter = 0;
  Config.strugCurrentPatternNumber = 0;
  EEPROM.put(0, Config);
  EEPROM.commit();
}
void putEEPROM(void) {
  Serial.printf("\nEEPROM geschrieben:\nvalidEEPROM = 0x%x, setModeLampe = %d, colorCounter = %d\npaletteCounter = %d, gCurrentPatternNumber = %d\n", validEEPROM, setModeLampe, colorCounter, paletteCounter, gCurrentPatternNumber);
  Config.valid = 0x55aa;
  Config.strusetModeLampe = setModeLampe;
  Config.strucolorCounter = colorCounter;
  Config.strupaletteCounter = paletteCounter;
  Config.strugCurrentPatternNumber = gCurrentPatternNumber;
  EEPROM.put(0, Config);
  EEPROM.commit();
}

void getEEPROM(void) {
  EEPROM.get(0, Config);
  validEEPROM = Config.valid;
  setModeLampe = Config.strusetModeLampe;
  colorCounter = Config.strucolorCounter;
  paletteCounter = Config.strupaletteCounter;
  gCurrentPatternNumber = Config.strugCurrentPatternNumber;
  Serial.printf("\nEEPROM gelesen:\nValidEEPROM = 0x%x, setModeLampe = %d, colorCounter = %d\npaletteCounter = %d, gCurrentPatternNumber = %d\n", validEEPROM, setModeLampe, colorCounter, paletteCounter, gCurrentPatternNumber);
}

void setup() {
  delay(1000); // power-up safety delay
  Serial.begin(115200);
  EEPROM.begin(128);
  Serial.printf("\nInitiale Werte:\nvalidEEPROM = 0x%x, setModeLampe = %d, colorCounter = %d\npaletteCounter = %d, gCurrentPatternNumber = %d\n", validEEPROM, setModeLampe, colorCounter, paletteCounter, gCurrentPatternNumber);
  pinMode(wakeUpPin, INPUT_PULLUP);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  Serial.println("\nfastled started");
  getEEPROM();
  if (validEEPROM != 0x55aa) {
    initEEPROM();
    Serial.println("EEPROM ungueltig");
    getEEPROM();
  }
  else {
    Serial.println("EEPROM gueltig");
    getEEPROM();
  }
  firstRun = true;
}

void loop() {
  FunctionButton.update();
  if (FunctionButton.isLongClick()) {
    if (!firstRun) {
      if (setModeLampe == 0) {
        colorCounter++;
        if (colorCounter > 17) {
          colorCounter = 0;
        }
      }
      else if (setModeLampe == 1) {
        paletteCounter++;
        if (paletteCounter > 11) { // adjust if you have more or less than 34 palettes
          paletteCounter = 0;
        }
      }
      else if (setModeLampe == 2) {
        nextPattern();  // Change to the next pattern
      }
      Serial.printf("\n**** SingleClick ***\nsetModeLampe = %d\n", setModeLampe);
      putEEPROM();
      getEEPROM();
    }
    else
      firstRun = false;
  }
  else if (FunctionButton.isDoubleClick()) {
    setModeLampe++;
    if (setModeLampe > 2) {
      setModeLampe = 0;
    }
    Serial.printf("\n+++ DoubleClick +++\nsetModeLampe = %d\n", setModeLampe);
    putEEPROM();
  }

  if (setModeLampe == 0) {
    if (colorCounter % 2 == 0) {
      float breath = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;
      FastLED.setBrightness(breath);
    }
    else {
      FastLED.setBrightness(BRIGHTNESS);
    }
    ChangeColorPeriodically();
  }
  else if (setModeLampe == 1) {
    FastLED.setBrightness(BRIGHTNESS);
    ChangePalettePeriodically();
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1;
    FillLEDsFromPaletteColors(startIndex);
  }
  else if (setModeLampe == 2) {
    gPatterns[gCurrentPatternNumber]();
  }

  FastLED.show();
  FastLED.delay(2000 / UPDATES_PER_SECOND);
  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }
}
