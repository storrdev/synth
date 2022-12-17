#include <FastLED.h>

// LEDs
FASTLED_USING_NAMESPACE

#define DATA_PIN 6
//#define CLK_PIN   4
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
const uint8_t NUM_LEDS = 64;
CRGB leds[NUM_LEDS];

#define BRIGHTNESS 3
#define FRAMES_PER_SECOND 120

// Rotary Encoder (Sequence Editing)
const uint8_t KNOBCLOCK = 2;
const uint8_t KNOBDATA = 3;
const uint8_t KNOBPUSH = 4;
uint8_t encoderPosCount = 0;
uint8_t knobClockLast;
uint8_t clockVal;
boolean bCW;
unsigned long lastKnobPush = 0;

// Rotary Encoder (Config Editing)
const uint8_t KNOBCLOCK2 = 7;
const uint8_t KNOBDATA2 = 5;
const uint8_t KNOBPUSH2 = 8;
uint8_t encoderPosCount2 = 0;
uint8_t knobClockLast2;
uint8_t clockVal2;
boolean bCW2;
unsigned long lastKnobPush2 = 0;

// BPM
unsigned long bpm = 80;
unsigned long lastBeat = millis();

byte steps = 8;
byte step = 0;
byte editStep = 0;

byte octave = 4

// Keys
byte aMaj[] = {
  0x45,  // A4
  0x44,  // G#3
  0x42,  // F#3
  0x40,  // E3
  0x3E,  // D3
  0x3D,  // C#3
  0x3B,  // B3
  0x39,  // A3
};

// Sequence
byte sequence[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

void updateEncoder() {
  // Read the current state of clock
  clockVal = digitalRead(KNOBCLOCK);

  // Serial.print("clockVal == 1: ");
  // Serial.println(clockVal == 1);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (clockVal != knobClockLast && clockVal == 1) {
    // Means the knob is rotating, we need to determine a direction
    // We do that by reading the data pin
    if (digitalRead(KNOBDATA) != clockVal) {  // Means clock pin changed first and we're rotating Clockwise
      sequence[editStep]++;
      encoderPosCount++;
      bCW = true;
    } else {  // Otherwise data changed first and we're moving CCW
      bCW = false;
      encoderPosCount--;
      sequence[editStep]--;
    }

    // Only update the sequence if we have a change
    // Serial.print("Encoder Position: ");
    // Serial.println(encoderPosCount);
    // Serial.print("sequence[editStep]: ");
    // Serial.println(sequence[editStep]);
  }

  knobClockLast = clockVal;  // Make sure this stays outside of the change checking conditional above

  // Knob push stuff

  // Read the button state
  int pushState = digitalRead(KNOBPUSH);

  // If we detect LOW signal, button is pressed
  if (pushState == LOW) {
    // if 50ms have passed since last LOW pulse, it means that the
    // button has been pressed, released and pressed again
    if (millis() - lastKnobPush > 50) {
      editStep++;
      if (editStep == 8) {
        editStep = 0;
      }
    }

    lastKnobPush = millis();
  }
}

void updateEncoder2() {
  // Read the current state of clock
  clockVal2 = digitalRead(KNOBCLOCK2);

  // Serial.print("clockVal == 1: ");
  // Serial.println(clockVal == 1);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (clockVal2 != knobClockLast2 && clockVal2 == 1) {
    // Means the knob is rotating, we need to determine a direction
    // We do that by reading the data pin
    if (digitalRead(KNOBDATA2) != clockVal2) {  // Means clock pin changed first and we're rotating Clockwise
      // sequence[editStep]++;
      bpm++;
      encoderPosCount2++;
      bCW2 = true;
    } else {  // Otherwise data changed first and we're moving CCW
      bCW2 = false;
      encoderPosCount2--;
      // sequence[editStep]--;
      bpm--;
    }

    // Only update the sequence if we have a change
    // Serial.print("Encoder Position: ");
    // Serial.println(encoderPosCount);
    // Serial.print("sequence[editStep]: ");
    // Serial.println(sequence[editStep]);
  }

  knobClockLast2 = clockVal2;  // Make sure this stays outside of the change checking conditional above

  // Knob push stuff

  // Read the button state
  int pushState = digitalRead(KNOBPUSH2);

  // If we detect LOW signal, button is pressed
  if (pushState == LOW) {
    // if 50ms have passed since last LOW pulse, it means that the
    // button has been pressed, released and pressed again
    if (millis() - lastKnobPush2 > 50) {
      // editStep++;
      // if (editStep == 8) {
      //   editStep = 0;
      // }
    }

    lastKnobPush2 = millis();
  }
}

// Theme
CRGB beatLineColor = CRGB::LightGrey;

void setup() {
  Serial.begin(9600);
  delay(3000);  // 3 second delay for recovery

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  if (millis() - lastBeat > 60000 / bpm) {
    lastBeat = millis();

    step++;
    if (step == steps) {
      step = 0;
    }
  }

  updateEncoder();
  updateEncoder2();

  FastLED.clear();

  // Draw "current beat" line that moves across the grid indicating which note is currently being played
  leds[step] = beatLineColor;
  leds[step + 8] = beatLineColor;
  leds[step + 16] = beatLineColor;
  leds[step + 24] = beatLineColor;
  leds[step + 32] = beatLineColor;
  leds[step + 40] = beatLineColor;
  leds[step + 48] = beatLineColor;
  leds[step + 56] = beatLineColor;

  // Draw the note sequence
  for (byte i = 0; i < steps; i++) {
    if (i == editStep) {
      leds[i + (sequence[i] * 8)] = CRGB::DarkRed;
    } else {
      leds[i + (sequence[i] * 8)] = CRGB::DarkBlue;
    }
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
  // FastLED.delay(1000/FRAMES_PER_SECOND);
}
