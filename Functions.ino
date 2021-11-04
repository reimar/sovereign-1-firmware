// ==================================================================================================================================
// program functions
// ==================================================================================================================================

void introSequence() {

    /**
     *  A simple intro sequence to test the LEDs and display
     *  the current firmware version number. Played once at
     *  start up.
     */

    if (playIntroSequence) {

        // display the start up message

        lcd.setCursor(0, 0);
        lcd.print("SOVEREIGN-1     ");
        lcd.setCursor(0, 1);
        lcd.print(softwareVersion);

        // let the LEDs blink, just for fun

        for(size_t i = 0; i < introSequencePlays; ++i) {
            lightLed(3); delay(introSequenceSpeed);
            lightLed(2); delay(introSequenceSpeed);
            lightLed(1); delay(introSequenceSpeed);
            lightLed(0); delay(introSequenceSpeed);
            lightLed(1); delay(introSequenceSpeed);
            lightLed(2); delay(introSequenceSpeed);
        }

        // switch all LEDs off

        switchAllLeds(LOW);

        // read the current song title from memory

        getSongTitle();

        // update display

        updateDisplay();

        // set flag to false to "exit" the intro sequence

        playIntroSequence = false;

    }

}

void saveData() {

   /**
    *  Save the data to the built-in EEPROM chip.
    *  The function detects changes and only saves
    *  data, if it actually changed in order to extend
    *  the EEPROM life span.
    */

    // set the save flag

    bool savedData = false;

    // get the index of the patch button in edit mode

    int patchIndex;

    for(size_t i = 0; i < totalPatchButtons; ++i) {
        if (editPatchSlot[i] == true) {
            patchIndex = i;
            break;
        }
    }

    // calculate the starting memory location for the patches

    int patchMemoryLocation = currentSong * (maxSongTitleLength + totalPatchButtons);

    // if the new patch is different from
    // the old one, save it

    if (newPatch != currentPatch) {
        currentPatch = newPatch;
        EEPROM.write(patchMemoryLocation + patchIndex, newPatch);
        savedData = true;
    }

    // calculate the starting memory location for the song title

    int songTitleMemoryLocation = patchMemoryLocation + totalPatchButtons + 1;

    // if the new song title is different from
    // the old one, save it

    for(size_t j = 0; j < maxSongTitleLength; ++j) {
        if (EEPROM.read(songTitleMemoryLocation + j) != songTitle[j]) {
            EEPROM.write(songTitleMemoryLocation + j, songTitle[j]);
            savedData = true;
        }
    }

    // display the confirmation message, no matter
    // if data was actually saved or not - for the
    // user, the result is the same

    displaySysMsg(1);

    // delay, so we have time to read the
    // confirmation message on the display

    delay(2000);

    // reset cursor

    resetCursorPos();

}

int getPatch(int patchIndex) {

   /**
    *  Return the patch for one of the patch slots from EEPROM memory.
    *  
    *  @param  {int} patchIndex - index number to select the right patch
    *  @return {int}            - the patch, read from memory
    */

    int memoryLocation = currentSong * (maxSongTitleLength + totalPatchButtons) + patchIndex;

    return EEPROM.read(memoryLocation);

}

void getSongTitle() {

   /**
    *  Read the song title from memory. No return, the values
    *  just get assigned to the songTitle character array.
    */

    int patchMemoryLocation     = currentSong * (maxSongTitleLength + totalPatchButtons);
    int songTitleMemoryLocation = patchMemoryLocation + totalPatchButtons + 1;

    for(size_t i = 0; i < maxSongTitleLength; ++i) {

        int valueFromMemory = EEPROM.read(songTitleMemoryLocation + i);

        if (valueFromMemory < 255) {
            songTitle[i] = valueFromMemory;
        } else {
            songTitle[i] = 0;
        }

    }

}

void lightLed(int ledIndex) {

   /**
    *  Switch a selected LED on.
    *
    *  @param {int} ledIndex - index number to select the right LED
    */

    for(size_t i = 0; i < totalPatchButtons; ++i) {

        if (i == ledIndex) {
            digitalWrite(patchLeds[i], HIGH);
        } else {
            digitalWrite(patchLeds[i], LOW);
        }

    }

}

void setBlinkInterval(int interval) {

   /**
    *  Sets a "blink" flag at a given interval speed. The flag
    *  is used by other functions to blink LEDs or the cursor
    *  in edit mode.
    *
    *  @param {int} interval - interval speed in ms
    */

    if (currentTimestamp - previousTimestamp >= interval) {

        previousTimestamp = currentTimestamp;

        if (blink == false) {
            blink = true;
        } else if(blink == true) {
            blink = false;
        }

    }

}

void blinkLed(int ledIndex) {

    /**
     *  Blink a selected LED at the interval, defined by setBlinkInterval().
     *
     *  @param {int} ledIndex - index number to select the right LED
     */

    if (blink) {
        patchLedsState[ledIndex] = HIGH;
    } else {
        patchLedsState[ledIndex] = LOW;
    }

    digitalWrite(patchLeds[ledIndex], patchLedsState[ledIndex]);

}

void switchAllLeds(bool state) {

   /**
    *  Switch all LEDs either on or off.
    *
    *  @param {bool} state - the LED state (on or off)
    */

    for(size_t i = 0; i < totalPatchButtons; ++i) {
        digitalWrite(patchLeds[i], state);
    }

}

void enterEditMode(int patchIndex) {

   /**
    *  Enter the edit mode. In edit mode, the patch number for each
    *  patch button as well as the title of the current song can be
    *  manipulated and saved.
    *
    *  @param {int} patchIndex - index number to select the right patch
    */

    // record current potentiometer value

    currentPotValue = analogRead(pot);

    // set patch slot

    editPatchSlot[patchIndex] = true;

    // set edit mode flag

    editMode = true;

}

void exitEditMode() {

   /**
    *  Exit the edit mode. Update the display and reset the
    *  cursor position.
    */

    // set edit mode flag

    editMode = false;

    // set patch slot flags

    for(size_t i = 0; i < totalPatchButtons; ++i) {
        editPatchSlot[i] = false;
    }

    // update display

    getSongTitle();
    updateDisplay();

    // reset cursor

    resetCursorPos();

}

void displaySongNumber(int songNumber) {

   /**
    *  Print a given song number to the display.
    *
    *  @param {int} songNumber - the given song number
    */

    // set the cursor and display the label

    lcd.setCursor(9, 1);
    lcd.print(" SNG");
    lcd.print(char(2));

    // print the song number, padded with leading zeros

    if (songNumber < 10) {
        lcd.print("0");
        lcd.print(songNumber);
    } else {
        lcd.print(songNumber);
        lcd.print(" ");
    }

}

void displaySelectPatchMsg() {
    
   /**
    *  Write a promt message to the display.
    */
    
    // set the cursor and display the message

    lcd.setCursor(0, 1);
    lcd.print("please select   ");
    
}

void displayPatchNumber(int patchNumber) {

   /**
    *  Print a given patch number to the display.
    *
    *  @param {int} patchNumber - the given patch number
    */

    // set the cursor and display the label

    lcd.setCursor(0, 1);
    lcd.print("PRG");
    lcd.print(char(2));

    // print the patch number, padded with leading zeros

    if (patchNumber < 128) {
        if (patchNumber < 10) {
            lcd.print("00");
            lcd.print(patchNumber);
        } else if (patchNumber < 100) {
            lcd.print("0");
            lcd.print(patchNumber);
        } else {
            lcd.print(patchNumber);
        }
    } else {
        lcd.print("---");
    }

}

void displayBankAndPatchNumber(int patchNumber) {

   /**
    *  Format and print a given patch number to the display.
    *  For synthesizers that group patches in banks.
    *  The formatting translates the absolute patchNumber (0-127)
    *  into a patch number in context of a bank number 
    *  (eg. 02/7 for "patch number 7 from bank 2") 
    *
    *  @param {int} patchNumber - the given patch number
    */

    // get the bank number

    int bank = (patchNumber / patchesPerBank) + 1;

    // get the patch number in context of the bank

    int patchInBank = (patchNumber % patchesPerBank) + 1;

    // set the cursor and display the label

    lcd.setCursor(0, 1);
    lcd.print("PRG");
    lcd.print(char(2));

    // display the formatted bank and patch number

    if (patchNumber < 128) {

        if (bank < 10) {
            lcd.print("0");
            lcd.print(bank);
        } else {
            lcd.print(bank);
        }

        lcd.print("/");
        lcd.print(patchInBank);

    } else {
        lcd.print("----");
    }

}

void displaySongTitle() {

   /**
    *  Print the title of the currently selected song to the display.
    */

    // set cursor and clear first line

    lcd.setCursor(0, 0);
    lcd.print("                ");

    // display song title

    for(size_t i = 0; i < strlen(songTitle); ++i) {
        lcd.setCursor(i, 0);
        lcd.print(songTitle[i]);
    }

}

void displaySysMsg(int messageIndex) {

   /**
    *  Print a system message to the display.
    *
    *  @param {int} messageId - index number to display the desired message
    */

    // message: "error"

    if (messageIndex == 0) {
        lcd.setCursor(0, 0);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("         ");
        lcd.print(char(1));
        lcd.print(" ERROR");
    }

    // message: "saved"

    if (messageIndex == 1) {
        lcd.setCursor(0, 0);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("         ");
        lcd.print(char(0));
        lcd.print(" SAVED");
    }

}

void updateDisplay() {

   /**
    *  Print the current values to the display.
    */

    displaySongTitle();
    displayBankAndPatchNumber(currentPatch);
    displaySongNumber(currentSong + 1);

}

void resetCursorPos() {

   /**
    *  Reset the cursor position.
    */

    cursorPos = maxSongTitleLength;

}

void sendMidiProgramChange() {

   /**
    *  Send the midi program change command.
    */

    if (currentPatch < 128 && (currentPatch != lastPatch)) {
        MIDI.sendProgramChange(currentPatch, midiChannel);
    }

}

void saveCurrentTimestamp() {

   /**
    *  Save the current time stamp.
    */

    currentTimestamp = millis();

}

bool potValueChanged() {

   /**
    *  Detect weather the potentiometer knob was turned.
    *
    *  @return {bool} - true if the pot value has changed, false if not
    */

    int diff = lastPotValue - analogRead(pot);

    return abs(diff) > 5;

}

void moveCursor() {

   /**
    *  Move the cursor forward by one step.
    */

    cursorPos = cursorPos < maxSongTitleLength ? ++cursorPos : 0;

}
