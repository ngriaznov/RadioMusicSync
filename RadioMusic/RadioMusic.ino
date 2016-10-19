/*
  RADIO MUSIC SYNC, based on:
  https://github.com/TomWhitwell/RadioMusic

  Audio out: Onboard DAC, teensy3.1 pin A14/DAC

  Bank Button: 2
  Bank LEDs 3,4,5,6
  Reset Button: 8
  Reset LED 11
  Reset CV input: 9
  Channel Pot: A9
  Channel CV: A8 // check
  Time Pot: A7
  Time CV: A6 // check
  SD Card Connections:
  SCLK 14
  MISO 12
  MOSI 7
  SS   10

  NB: Compile using modified versions of:
  SD.cpp (found in the main Arduino package)
  play_sd_raw.cpp  - In Teensy Audio Library
  play_sc_raw.h    - In Teensy Audio Library

  from:https://github.com/TomWhitwell/RadioMusic/tree/master/Collateral/Edited%20teensy%20files

*/
#include <EEPROM.h>
#include <Bounce.h>
#include <Audio.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

// REBOOT CODES
#define                     RESTART_ADDR 0xE000ED0C
#define                     READ_RESTART() (*(volatile uint32_t *)RESTART_ADDR)
#define                     WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

// SETUP VARS TO STORE DETAILS OF FILES ON THE SD CARD
#define                     MAX_FILES 75
#define                     BANKS 16

// SETUP VARS TO STORE CONTROLS
#define                     CHAN_POT_PIN A9                               // pin for Channel pot
#define                     CHAN_CV_PIN A6                                // pin for Channel CV
#define                     TIME_POT_PIN A7                               // pin for Time pot
#define                     TIME_CV_PIN A8                                // pin for Time CV
#define                     RESET_BUTTON 8                                // Reset button
#define                     RESET_LED 11                                  // Reset LED indicator
#define                     RESET_CV 9                                    // Reset pulse input

// BANK SWITCHER SETUP
#define                     BANK_BUTTON 2                                 // Bank Button
#define                     LED0 6
#define                     LED1 5
#define                     LED2 4
#define                     LED3 3
#define                     BANK_SAVE 0

#define                     peakFPS 12                                    //  FRAMERATE FOR PEAK METER

#define                     FLASHTIME 10                                  // How long do LEDs flash for?
#define                     HOLDTIME 400                                  // How many millis to hold a button to get 2ndary function?

// OPTIONS TO READ FROM THE SD CARD, WITH DEFAULT VALUES
boolean                     MUTE = false;                                 // Softens clicks when changing channel / position, at
                                                                          // cost of speed. Fade speed is set by DECLICK
int                         DECLICK = 2;                                  // milliseconds of fade in/out on switching
boolean                     ShowMeter = true;                             // Does the VU meter appear?
int                         meterHIDE = 2000;                             // how long to show the meter after bank change in Milliseconds
boolean                     ChanPotImmediate = true;                      // Settings for Pot / CV response.
boolean                     ChanCVImmediate = true;                       // TRUE means it jumps directly when you move or change.
boolean                     StartPotImmediate = false;                    // FALSE means it only has an effect when RESET is pushed or triggered
boolean                     StartCVImmediate = false;
int                         StartCVDivider = 2;                           // Changes sensitivity of Start control. 1 = most sensitive, 512 = lest sensitive (i.e only two points)
boolean                     Looping = true;                               // When a file finishes, start again from the beginning
int                         currentTimePosition = 0;
int                         BPM = 130;                                    // Base BPM for loops
int                         skipTransition = 0;

File                        settingsFile;

AudioMixer4                 mixer;
AudioPlaySdRaw              playRaw1;  
AudioPlaySdRaw              playRaw2;  
AudioEffectFade             fade1;    
AudioEffectFade             fade2;
AudioAnalyzePeak            peak1;
AudioOutputAnalog           dac1;
AudioConnection             patchCord1(playRaw1, fade1);
AudioConnection             patchCord4(playRaw2, fade2);
AudioConnection             patchCord7(mixer, dac1);
AudioConnection             patchCord2(fade1, 0, mixer, 0);
AudioConnection             patchCord6(fade2, 0, mixer, 1);
AudioConnection             patchCord3(mixer, peak1);

int                         ACTIVE_BANKS;
String                      FILE_TYPE = "RAW";
String                      FILE_NAMES[BANKS][MAX_FILES];
String                      FILE_DIRECTORIES[BANKS][MAX_FILES];
unsigned long               FILE_SIZES[BANKS][MAX_FILES];
int                         FILE_COUNT[BANKS];
String                      CURRENT_DIRECTORY = "0";
File                        root;

boolean                     CHAN_CHANGED = true;
boolean                     RESET_CHANGED = false;
boolean                     CLOCK_CHANGED = false;
unsigned long               PLAY_POSITION = 0;
unsigned long               SYNC_POSITION = 0;

Bounce                      resetSwitch = Bounce(RESET_BUTTON, 20);       // Bounce setup for Reset
int                         PLAY_CHANNEL;
int                         NEXT_CHANNEL;
unsigned long               playhead;
char                        *targetFile;


Bounce                      bankSwitch = Bounce(BANK_BUTTON, 20);
int                         PLAY_BANK = 0;

// CHANGE HOW INTERFACE REACTS
int                         chanHyst = 3;                                 // how many steps to move before making a change (out of 1024 steps on a reading)
int                         timHyst = 6;

elapsedMillis               chanChanged;
elapsedMillis               timChanged;
elapsedMillis               clockTime;

int                         sampleAverage = 40;
int                         chanPotOld;
int                         chanCVOld;
int                         timPotOld;
int                         timCVOld;

elapsedMillis               showDisplay;                                  // elapsedMillis is a special variable in Teensy - increments every millisecond
int                         showFreq = 250;                               // how many millis between serial Debug updates
elapsedMillis               resetLedTimer = 0;
elapsedMillis               bankTimer = 0;
elapsedMillis               checkI = 0;                                   // check interface
int                         checkFreq = 10;                               // how often to check the interface in Millis

// CONTROL THE PEAK METER DISPLAY
elapsedMillis               meterDisplay;                                 // Counter to hide MeterDisplay after bank change
elapsedMillis               fps;                                          // COUNTER FOR PEAK METER FRAMERATE
bool                        fadeSwitch = false;

void setup() {
  // PINS FOR BANK SWITCH AND LEDS
  pinMode(BANK_BUTTON, INPUT);
  pinMode(RESET_BUTTON, INPUT);
  pinMode(RESET_CV, INPUT);
  pinMode(CHAN_CV_PIN, INPUT);
  pinMode(RESET_LED, OUTPUT);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  ledWrite(PLAY_BANK);

  // START SERIAL MONITOR
  Serial.begin(38400);

  // MEMORY REQUIRED FOR AUDIOCONNECTIONS
  AudioMemory(7);
  // SD CARD SETTINGS FOR AUDIO SHIELD
  SPI.setMOSI(7);
  SPI.setSCK(14);

  // OPEN SD CARD
  int crashCountdown;
  if (!(SD.begin(10))) {
    while (!(SD.begin(10))) {
      ledWrite(15);
      delay(100);
      ledWrite(0);
      delay(100);
      crashCountdown++;
      if (crashCountdown > 6) reBoot();
    }
  }

  mixer.gain(0, 0.7);
  mixer.gain(1, 0.7);

  // READ SETTINGS FROM SD CARD
  root = SD.open("/");

  if (SD.exists("settings.txt")) {
    readSDSettings();
  } else {
    writeSDSettings();
  };

  // OPEN SD CARD AND SCAN FILES INTO DIRECTORY ARRAYS
  scanDirectory(root, 0);

  // CHECK  FOR SAVED BANK POSITION
  int a = 0;
  a = EEPROM.read(BANK_SAVE);
  if (a >= 0 && a <= ACTIVE_BANKS) {
    PLAY_BANK = a;
    CHAN_CHANGED = true;
  } else {
    EEPROM.write(BANK_SAVE, 0);
  };

  // Add an interrupt on the RESET_CV pin to catch rising edges
  attachInterrupt(RESET_CV, resetcv, RISING);
  attachInterrupt(CHAN_CV_PIN, clockrecieve, RISING);

  // Silence second player for now
  fade1.fadeOut(2);

  skipTransition = round(44.1 * 30000/BPM); // ((60000/BPM/4)*2 * (44100/1000))
  clockTime = 0;
}

// Called by interrupt on rising edge, for RESET_CV pin
void resetcv() { 
  RESET_CHANGED = true; 
}

// Called when station recieves clock
void clockrecieve() { 
  CLOCK_CHANGED = true; 
}

void loop() {
  if (SYNC_POSITION == 0){
    SYNC_POSITION = currentTimePosition;
  }
  
  if (!playRaw1.isPlaying() && Looping) {
    targetFile = buildPath(PLAY_BANK, PLAY_CHANNEL);
    playRaw1.playFrom(targetFile, 0);
  }

  if (!playRaw2.isPlaying() && Looping) {
    targetFile = buildPath(PLAY_BANK, PLAY_CHANNEL);
    playRaw2.playFrom(targetFile, 0);       
  }

  resetLedTimer = 0;  

  if (playRaw1.failed) {
    reBoot();
  }

  if (CHAN_CHANGED) {
    targetFile = buildPath(PLAY_BANK, NEXT_CHANNEL);
    PLAY_CHANNEL = NEXT_CHANNEL;

    if (RESET_CHANGED == false && Looping)
      PLAY_POSITION = currentTimePosition;  
      PLAY_POSITION = (PLAY_POSITION / 16) * 16; 
     
      playRaw1.playFrom(targetFile, PLAY_POSITION);        // change audio
      playRaw2.playFrom(targetFile, PLAY_POSITION);        // change audio

      ledWrite(PLAY_BANK);
      CHAN_CHANGED = false;
      resetLedTimer = 0;  // turn on Reset LED
  }

  if (RESET_CHANGED){
    PLAY_POSITION = currentTimePosition;  
    SYNC_POSITION = PLAY_POSITION;
    RESET_CHANGED = false;
  }
  
  if (CLOCK_CHANGED || RESET_CHANGED) { 
    targetFile = buildPath(PLAY_BANK, PLAY_CHANNEL);
    PLAY_POSITION = (PLAY_POSITION / 16) * 16; 
    
    if (fadeSwitch){            
      playRaw2.playFrom(targetFile, PLAY_POSITION);
      fade1.fadeOut(clockTime * 0.1);
      fade2.fadeIn(clockTime * 0.1);          
    }
    else {
      playRaw1.playFrom(targetFile, PLAY_POSITION);
      fade1.fadeIn(clockTime * 0.1);
      fade2.fadeOut(clockTime * 0.1);
    }

    fadeSwitch = !fadeSwitch;
    SYNC_POSITION += skipTransition;
    
    PLAY_POSITION = SYNC_POSITION; 
    CLOCK_CHANGED = false;
    RESET_CHANGED = false;
    clockTime = 0;
  }

  if (checkI >= checkFreq) {
    checkInterface();
    checkI = 0;
  }

  if (showDisplay > showFreq) {
    showDisplay = 0;
  }

  digitalWrite(RESET_LED, resetLedTimer < FLASHTIME);  // flash reset LED

  if (fps > 1000 / peakFPS && meterDisplay > meterHIDE && ShowMeter)
    peakMeter();  // CALL PEAK METER
}

