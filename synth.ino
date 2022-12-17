#include <Adafruit_NeoPixel.h>
#include <MIDI.h>
// #include <MozziGuts.h>
#include <Metronome.h>
#include <Oscil.h>                // oscillator template
#include <tables/sin2048_int8.h>  // sine table for oscillator
#include <mozzi_midi.h>
#include <ADSR.h>

// Input knob stuff
const uint8_t KNOBCLOCK = 2;
const uint8_t KNOBDATA = 3;
const uint8_t KNOBPUSH = 4;
uint8_t encoderPosCount = 0;
uint8_t knobClockLast;
uint8_t clockVal;
boolean bCW;
unsigned long lastKnobPush = 0;
// const uint8_t VOLUME = 0;

// Neopixel stuff
#define NEOPIXELPIN 6  // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 64   // Neopixel Shield Size

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);

// MIDI/Mozzi Stuff

MIDI_CREATE_DEFAULT_INSTANCE();

// use #define for CONTROL_RATE, not a constant
// #define CONTROL_RATE 128  // Hz, powers of 2 are most reliable

// audio sinewave oscillator
// Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

// envelope generator
// ADSR<CONTROL_RATE, AUDIO_RATE> envelope;

// metronome
Metronome kMetro(60000 / 120);

// to convey the volume level from updateControl() to updateAudio()
// byte volume;

#define LED 13  // shows if MIDI is being recieved

// Keys
// byte aMaj[] = { 0x39, 0x3B, 0x3D, 0x3E, 0x40, 0x42, 0x44, 0x45 };  // A B C# D E F# G# A(octave) start at A3 for now https://www.inspiredacoustics.com/en/MIDI_note_numbers_and_center_frequencies
byte aMaj[] = {
  0x45,  // A4
  0x44,  // G#3
  0x42,  // F#3
  0x40,  // E3
  0x3E,  // D3
  0x3D,  // C#3
  0x3B,  // B3
  0x39,  // A3
};       // (reverse so it makes sense with the pixel grid)

// Sequencer stuff
const byte steps = 8;
byte sequence[steps] = { 7, 7, 7, 2, 7, 5, 4, 3 };
byte step = 0;
byte editStep = 0;
byte note;

// Functions

void HandleNoteOn(byte channel, byte note, byte velocity) {
  aSin.setFreq(mtof(float(note)));
  envelope.noteOn();
  digitalWrite(LED, HIGH);
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  envelope.noteOff();
  digitalWrite(LED, LOW);
}

// Interrupt handlers
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

// Setup
void setup() {
  // Input knob setup
  pinMode(KNOBCLOCK, INPUT);
  pinMode(KNOBDATA, INPUT);

  // Read clock pin - Whatever state it's in will reflect the last position
  knobClockLast = digitalRead(KNOBCLOCK);

  // Neopixels
  pinMode(LED, OUTPUT);

  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  // MIDI.setHandleNoteOn(HandleNoteOn);    // Put only the name of the function
  // MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  // Initiate MIDI communications, listen to all channels (not needed with Teensy usbMIDI)
  // MIDI.begin(MIDI_CHANNEL_OMNI);

  // envelope.setADLevels(255, 64);
  // envelope.setTimes(50, 200, 10000, 200);  // 10000 is so the note will sustain 10 seconds unless a noteOff comes

  aSin.setFreq(440);  // default frequency

  kMetro.start();  // start metronome

  startMozzi(CONTROL_RATE);
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)

  // These might be necessary but I don't want to waste interrupts if I don't have too, and everything is working right now
  // attachInterrupt(0, updateEncoder, CHANGE); // pin 2
  // attachInterrupt(1, updateEncoder, CHANGE); // pin 3

  // Serial.begin(9600);
}

// Update

void updateControl() {
  pixels.clear();  // Set all pixel colors to 'off'
  MIDI.read();

  // read the variable resistor for volume
  int sensor_value = mozziAnalogRead(VOLUME); // value is 0-1023

  // map it to an 8 bit range for efficient calculations in updateAudio
  volume = map(sensor_value, 0, 1023, 0, 255);

  updateEncoder();

  if (kMetro.ready()) {
    note = aMaj[sequence[step]];
    aSin.setFreq(mtof(float(note)));
    envelope.noteOn();

    // Draw line vertically representing what beat we're on
    pixels.setPixelColor(step, pixels.Color(0, 0, 3));
    pixels.setPixelColor(step + 8, pixels.Color(0, 0, 3));
    pixels.setPixelColor(step + 16, pixels.Color(0, 0, 3));
    pixels.setPixelColor(step + 24, pixels.Color(0, 0, 3));
    pixels.setPixelColor(step + 32, pixels.Color(0, 0, 3));
    pixels.setPixelColor(step + 40, pixels.Color(0, 0, 3)); // Leaving these out because I'm testing a smaller grid than I would build the actual synth with
    pixels.setPixelColor(step + 48, pixels.Color(0, 0, 3));
    pixels.setPixelColor(step + 56, pixels.Color(0, 0, 3));

    // Draw the note sequence
    for (byte i = 0; i < steps; i++) {
      if (i == editStep) {
        pixels.setPixelColor(i + (sequence[i] * 8), pixels.Color(10, 0, 0));
      } else {
        pixels.setPixelColor(i + (sequence[i] * 8), pixels.Color(0, 10, 5));
      }
    }

    pixels.show();  // Send the updated pixel colors to the hardware.

    step++;
    if (step == steps) {
      step = 0;
    }
  } else {
    envelope.noteOff();
  }

  envelope.update();
  envelope.next();
}

AudioOutput_t updateAudio() {
  // return MonoOutput::from16Bit((envelope.next() * volume) * aSin.next());
  return MonoOutput::from16Bit((int)aSin.next() * volume); // 8 bit * 8 bit gives 16 bits value
}

// Loop

void loop() {
  audioHook();  // required here
}