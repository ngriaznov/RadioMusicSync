//////////////////////////////////////////
// SCAN SD DIRECTORIES INTO ARRAYS //////
/////////////////////////////////////////
void scanDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    String fileName = entry.name();

    if (fileName.endsWith(FILE_TYPE) && fileName.startsWith("_") == 0 &&
        fileName.startsWith("COPY_") == 0) {
      int intCurrentDirectory = CURRENT_DIRECTORY.toInt();
      if (intCurrentDirectory > ACTIVE_BANKS)
        ACTIVE_BANKS = intCurrentDirectory;
      FILE_NAMES[intCurrentDirectory][FILE_COUNT[intCurrentDirectory]] =
          entry.name();
      FILE_SIZES[intCurrentDirectory][FILE_COUNT[intCurrentDirectory]] =
          entry.size();
      FILE_DIRECTORIES[intCurrentDirectory][FILE_COUNT[intCurrentDirectory]] =
          CURRENT_DIRECTORY;
      FILE_COUNT[intCurrentDirectory]++;
    };
    // Ignore OSX Spotlight and Trash Directories
    if (entry.isDirectory() && fileName.startsWith("SPOTL") == 0 &&
        fileName.startsWith("TRASH") == 0) {
      CURRENT_DIRECTORY = entry.name();
      scanDirectory(entry, numTabs + 1);
    }
    entry.close();
  }
}

// TAKE BANK AND CHANNEL AND RETURN PROPERLY FORMATTED PATH TO THE FILE
char* buildPath(int bank, int channel) {
  String liveFilename = "";
  liveFilename += bank;
  liveFilename += "/";
  liveFilename += FILE_NAMES[bank][channel];
  char filename[liveFilename.length() + 1];
  liveFilename.toCharArray(filename, sizeof(filename));
  return filename;
}

void reBoot() {
  delay(500);
  WRITE_RESTART(0x5FA0004);
}

// DISPLAY PEAK METER IN LEDS
void peakMeter() {
  if (peak1.available()) {
    fps = 0;
    float peakReading = peak1.read();
    int monoPeak = round(peakReading * 4);
    monoPeak = round(pow(2, monoPeak));
    ledWrite(monoPeak - 1);  //
  }
}

// WRITE A 4 DIGIT BINARY NUMBER TO LED0-LED3
void ledWrite(int n) {
  digitalWrite(LED0, HIGH && (n & B00001000));
  digitalWrite(LED1, HIGH && (n & B00000100));
  digitalWrite(LED2, HIGH && (n & B00000010));
  digitalWrite(LED3, HIGH && (n & B00000001));
}

//////////////////////////////////////////////
////READ AND WRITE SETTINGS ON THE SD CARD ///
////VIA http://overskill.alexshu.com/?p=107 ///
//////////////////////////////////////////////

void readSDSettings() {
  char character;
  String settingName;
  String settingValue;
  File settingsFile = SD.open("settings.txt");
  if (settingsFile) {
    while (settingsFile.available()) {
      character = settingsFile.read();
      while (character != '=') {
        settingName = settingName + character;
        character = settingsFile.read();
      }
      character = settingsFile.read();
      while (character != '\n') {
        settingValue = settingValue + character;
        character = settingsFile.read();
        if (character == '\n') {
          /*
            //Debuuging Printing
            Serial.print("Name:");
            Serial.println(settingName);
            Serial.print("Value :");
            Serial.println(settingValue);
          */
          // Apply the value to the parameter
          applySetting(settingName, settingValue);
          // Reset Strings
          settingName = "";
          settingValue = "";
        }
      }
    }
    // close the file:
    settingsFile.close();
  } else {
    // error
  }
}
/* Apply the value to the parameter by searching for the parameter name
  Using String.toInt(); for Integers
  toFloat(string); for Float
  toBoolean(string); for Boolean
*/
void applySetting(String settingName, String settingValue) {

  if (settingName == "DECLICK") {
    DECLICK = settingValue.toInt();
  }

  if (settingName == "ShowMeter") {
    ShowMeter = toBoolean(settingValue);
  }

  if (settingName == "meterHIDE") {
    meterHIDE = settingValue.toInt();
  }

  if (settingName == "ChanPotImmediate") {
    ChanPotImmediate = toBoolean(settingValue);
  }

  if (settingName == "ChanCVImmediate") {
    ChanCVImmediate = toBoolean(settingValue);
  }

  if (settingName == "StartPotImmediate") {
    StartPotImmediate = toBoolean(settingValue);
  }

  if (settingName == "StartCVImmediate") {
    StartCVImmediate = toBoolean(settingValue);
  }

  if (settingName == "StartCVDivider") {
    StartCVDivider = settingValue.toInt();
  }

  if (settingName == "Looping") {
    Looping = toBoolean(settingValue);
  }

  if (settingName == "BPM") {
    BPM = settingValue.toInt();
  }
}
// converting string to Float
float toFloat(String settingValue) {
  char floatbuf[settingValue.length()];
  settingValue.toCharArray(floatbuf, sizeof(floatbuf));
  float f = atof(floatbuf);
  return f;
}
// Converting String to integer and then to boolean
// 1 = true
// 0 = false
boolean toBoolean(String settingValue) {
  if (settingValue.toInt() == 1) {
    return true;
  } else {
    return false;
  }
}
// Writes A Configuration file
void writeSDSettings() {
  // Create new one
  File settingsFile = SD.open("settings.txt", FILE_WRITE);
  // writing in the file works just like regular print()/println() function
  settingsFile.print("DECLICK=");
  settingsFile.println(DECLICK);
  settingsFile.print("ShowMeter=");
  settingsFile.println(ShowMeter);
  settingsFile.print("meterHIDE=");
  settingsFile.println(meterHIDE);
  settingsFile.print("ChanPotImmediate=");
  settingsFile.println(ChanPotImmediate);
  settingsFile.print("ChanCVImmediate=");
  settingsFile.println(ChanCVImmediate);
  settingsFile.print("StartPotImmediate=");
  settingsFile.println(StartPotImmediate);
  settingsFile.print("StartCVImmediate=");
  settingsFile.println(StartCVImmediate);
  settingsFile.print("StartCVDivider=");
  settingsFile.println(StartCVDivider);
  settingsFile.print("Looping=");
  settingsFile.println(Looping);
  settingsFile.print("BPM=");
  settingsFile.println(BPM);
  
  // close the file:
  settingsFile.close();
}

