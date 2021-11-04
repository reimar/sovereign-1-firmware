// ==================================================================================================================================
// include libraries
// ==================================================================================================================================

#include <Wire.h>
#include <EEPROM.h>
#include <MIDI.h>
#include <LiquidCrystal_I2C.h>

// ==================================================================================================================================
// initialize library instances
// ==================================================================================================================================

// due to different chip sets, different LCD display driver modules
// may have different addresses:
// * (0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE) (my blue display)
// * (0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE) (my red display)

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// create a midi instance

MIDI_CREATE_DEFAULT_INSTANCE();

// ==================================================================================================================================
// program configuration
// ==================================================================================================================================

// software version

char softwareVersion[] = "v0.1.1";

// settings

const int introSequencePlays = 4;                           // number of times the intro LED "animation" will play
const int introSequenceSpeed = 100;                         // intro sequence playback speed in ms

const int eepromSize         = 1024;                        // size of in-build EEPROM
const int totalPatchButtons  = 4;                           // number of patch buttons
const int totalSongs         = 50;                          // number of songs
const int maxSongTitleLength = 16;                          // maximum song title length
const int midiChannel        = 1;                           // the midi channel sovereign uses to communicate with your synthesizer
const int patchesPerBank     = 8;                           // if your synthesizer groups patches into banks, set the number of patches per bank

const int patchButtons[4]    = { 2, 3, 4, 5 };              // patch button pins
const int patchLeds[4]       = { 6, 7, 8, 9 };              // patch LED pins
const int songButtons[2]     = { 10, 11 };                  // song button pins
const int pot                = 0;                           // potentiometer pin

// variables

char songTitle[maxSongTitleLength + 1];                     // song title
char currentCharacter;                                      // currently selected character of the song title
char lastCharacter;                                         // last selected character of the song title
int cursorPos = maxSongTitleLength;                         // cursor start-position for edit mode
int currentPotValue;                                        // current potentiometer value
int lastPotValue;                                           // last potentiometer value
int currentSong = 0;                                        // current song (always start with first song)
int currentPatch;                                           // current patch
int lastPatch = 128;                                        // last patch
int newPatch;                                               // a new patch to store

// look-up tables

int buttonHoldtime[4] = { 0, 0, 0, 0 };                     // array to store how long each patch button is held down
bool editPatchSlot[4] = { false, false, false, false };     // array to store the edit status of each patch slot
int patchLedsState[4] = { LOW, LOW, LOW, LOW };             // array to store the status of each LED

// timestamps

unsigned long previousTimestamp = 0;                        // the timestamp of the last loop
unsigned long currentTimestamp  = 0;                        // the timestamp of the current loop

// flags

bool playIntroSequence = true;                              // flag to display a intro sequence on start up
bool editMode          = false;                             // flag for edit mode
bool blink             = false;                             // flag for blinking (cursor or LEDs)

// character lookup-table for song titles

char characterTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789- ";

// custom characters

byte checkMark[8]    = { 0b00000, 0b00000, 0b00001, 0b00010, 0b10100, 0b01000, 0b00000, 0b00000 };
byte errorCross[8]   = { 0b00000, 0b00000, 0b01010, 0b00100, 0b01010, 0b00000, 0b00000, 0b00000 };
byte chevronRight[8] = { 0b00000, 0b00000, 0b00100, 0b00010, 0b00100, 0b00000, 0b00000, 0b00000 };

// ==================================================================================================================================
// ARDUINO setup()
// ==================================================================================================================================

void setup() {

    // since we only SEND midi commands, turn
    // off listening to midi commands

    MIDI.begin(MIDI_CHANNEL_OFF);

    // * to control an instrument via midi in, the serial port
    //   needs to run at the same baud rate as midi: 31250
    //   this is the default value, already set by MIDI.begin()
    //   so there is no need to set the serial to 31250
    // * for debugging via Arduino IDE serial monitor, set the
    //   baud rate to: 9600
    // * for monitoring midi messages via usb-midi-interface,
    //   change the baud rate to: 115200
    //
    // Serial.begin(115200);

    // set up patch buttons, patch leds and song buttons

    for(size_t i = 0; i < totalPatchButtons; ++i) {
        pinMode(patchButtons[i], INPUT);
    }

    for(size_t i = 0; i < totalPatchButtons; ++i) {
        pinMode(patchLeds[i], OUTPUT);
    }

    for(size_t i = 0; i < 2; ++i) {
        pinMode(songButtons[i], INPUT);
    }

    // set up display

    lcd.begin(16,2);
    lcd.backlight();

    // register custom characters

    lcd.createChar(0, checkMark);
    lcd.createChar(1, errorCross);
    lcd.createChar(2, chevronRight);

}

// ==================================================================================================================================
// ARDUINO loop()
// ==================================================================================================================================

void loop() {

    // play intro sequence

    introSequence();
    
    // save current timestamp

    saveCurrentTimestamp();
    
    // start blink interval

    setBlinkInterval(350);

    // patch buttons
    // -------------

    for(size_t i = 0; i < totalPatchButtons; ++i) {

        // boolean helpers, we need them to combine
        // them with other booleans in the following
        // conditional statements

        bool btnPressed  = digitalRead(patchButtons[i]) == 1 ? true : false;
        bool btnReleased = digitalRead(patchButtons[i]) == 0 ? true : false;

        if (editMode == false) {

            if (btnPressed && editPatchSlot[i] == false) {

                // save last potentiometer value

                lastPotValue = analogRead(pot);

                // read patch from memory

                currentPatch = getPatch(i);

                // send midi program change command

                sendMidiProgramChange();

                // light up corresponding led

                lightLed(i);

                // read current song title from memory

                getSongTitle();

                // update display

                updateDisplay();

                // record last patch

                lastPatch = currentPatch;

                // record button hold time

                ++buttonHoldtime[i];

                // enter edit mode after long press

                if (buttonHoldtime[i] > 15) enterEditMode(i);

            } else if (btnReleased && editPatchSlot[i] == false) {

                // reset button hold time

                buttonHoldtime[i] = 0;

            }

        } else if (editMode == true) {
            
            if (btnReleased && editPatchSlot[i] == true) {

                // reset button hold time

                buttonHoldtime[i] = 0;

            } else if (btnPressed && editPatchSlot[i] == false) {

                // if any other patch button is pressed,
                // exit edit mode

                exitEditMode();

            }

        }

    }

    // "song-up" or "save" button
    // --------------------------

    if (digitalRead(songButtons[0]) == 1) {

        if (editMode == true) {

            // if in edit mode - save patch and exit edit mode

            saveData();
            exitEditMode();

        } else if (currentSong < totalSongs - 1) {

            // switch off LEDs

            switchAllLeds(LOW);

            // increase song index

            ++currentSong;

            // read current song title from memory

            getSongTitle();

            // update display

            updateDisplay();
            
            // promt the user to select a bank/patch
            
            displaySelectPatchMsg();

            // short delay for robustness

            delay(200);

        }

    }

    // "song-down" or "cursor" button
    // ------------------------------

    if (digitalRead(songButtons[1]) == 1) {

        if (editMode == true) {

            // save last potentiometer value

            lastPotValue = analogRead(pot);

            // update display

            updateDisplay();

            // move cursor position

            moveCursor();

            // short delay for robustness

            delay(200);

        } else if (currentSong > 0) {

            // switch off LEDs

            switchAllLeds(LOW);

            // decrease song index

            --currentSong;

            // read current song title from memory

            getSongTitle();

            // update display

            updateDisplay();
            
            // promt the user to select a bank/patch
            
            displaySelectPatchMsg();

            // short delay for robustness

            delay(200);

        }

    }

    // edit mode
    // ---------
    
    // update song title at current cursor position

    if (editMode == true) {

        // blink patch LED

        for(size_t i = 0; i < totalPatchButtons; ++i) {
            if (editPatchSlot[i] == true) blinkLed(i);
        }

        // edit patch number
        // -----------------

        if (cursorPos == maxSongTitleLength) {

            if (potValueChanged()) {

                // after pot knob position changed,
                // read pot value and define new patch

                currentPotValue = analogRead(pot) / 7.9;
                newPatch        = currentPotValue < 128 ? currentPotValue : 128;

            } else {

                // however, if pot knob position did not change,
                // display last patch

                newPatch = currentPatch;

            }

            if (blink) {

                // blink patch number

                lcd.setCursor(4, 1);
                lcd.print("    ");

            } else {

                // if the synthesizer groups patches into banks (see setting at the top),
                // display selected bank and patch number - otherwise display the patch
                // as an absolute number between 0-127.

                if ((bool)patchesPerBank) {
                    displayBankAndPatchNumber(newPatch);
                } else {
                    displayPatchNumber(newPatch);
                }

            }

        // edit song title
        // ---------------

        } else {

            if (potValueChanged()) {

                // after pot knob position changed,
                // read pot value and define current character

                currentPotValue      = analogRead(pot) / 16;
                currentCharacter     = characterTable[currentPotValue];
                songTitle[cursorPos] = currentCharacter;

            } else {

                // however, if pot knob position did not change,
                // display last character

                currentCharacter = songTitle[cursorPos];

            }

            if (blink) {

                // blink cursor

                lcd.setCursor(cursorPos, 0);
                lcd.print(char(255));

            } else {

                // display current character

                lcd.setCursor(cursorPos, 0);
                lcd.print(currentCharacter);

            }

        }

    }

}
