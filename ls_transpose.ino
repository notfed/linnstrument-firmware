// -------------------------------
// TODO:
// -  Handle split-screen
// -------------------------------

boolean transpose2Enabled = true;

boolean isTranspose2Enabled() {
  return transpose2Enabled;
}

boolean setIsTranspose2Enabled(boolean value) {
  transpose2Enabled = value;
}

short downCount = 0;

byte prevDownSensorCol = -2;
byte prevDownSensorRow = -2;

byte prevUpSensorCol = -2;
byte prevUpSensorRow = -2;


// signed char transposeOctave;            // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
// signed char transposePitch;             // transpose output midi notes. Range is -12 to +12
// signed char transposeLights;            // transpose lights on display. Range is -12 to +12

void transpose2PaintOctaveTransposeDisplay() {
  blinkAllRootNotes = true;
  paintNormalDisplay();
  blinkAllRootNotes = false;
}

void transpose2HandleOctaveTransposeNewTouch() {
  addOrRemovePress(1);
  colorPress();

  if (didSingleDragLeft()) {
    // Split[Global.currentPerSplit].transposePitch++;
    Split[Global.currentPerSplit].transposeLights--;
    normalizeTranspose();
  }
  else if (didSingleDragRight()) {
    // Split[Global.currentPerSplit].transposePitch--;
    Split[Global.currentPerSplit].transposeLights++;
    normalizeTranspose();
  }

  // Save cell for next event
  prevDownSensorCol = sensorCol;
  prevDownSensorRow = sensorRow;

  updateDisplay();
}

void normalizeTranspose() {
    // if (Split[Global.currentPerSplit].transposePitch == -12) {
    //     Split[Global.currentPerSplit].transposePitch = 0;
    //     Split[Global.currentPerSplit].transposeOctave -= 12;
    // }
    // if (Split[Global.currentPerSplit].transposePitch == 12) {
    //     Split[Global.currentPerSplit].transposePitch = 0;
    //     Split[Global.currentPerSplit].transposeOctave += 12;
    // }
    // Split[Global.currentPerSplit].transposeLights = mod(Split[Global.currentPerSplit].transposeLights, 12);
    // if (Split[Global.currentPerSplit].transposeOctave < -60) {
    //     Split[Global.currentPerSplit].transposeOctave = -60;
    // }
    // if (Split[Global.currentPerSplit].transposeOctave > 60) {
    //     Split[Global.currentPerSplit].transposeOctave = 60;
    // }
}

void transpose2HandleOctaveTransposeRelease() {
  addOrRemovePress(-1);
  colorPress();

  // Save cell for next event
  prevUpSensorCol = sensorCol;
  prevUpSensorRow = sensorRow;

  // updateDisplay();
}

void addOrRemovePress(short delta) {
    downCount += delta; 
}

void colorPress() {
  // switch (downCount) {
  //   case 0:
  //       setLed(sensorCol, sensorRow, COLOR_BLACK, cellOff);
  //       break;
  //   case 1:
  //       setLed(sensorCol, sensorRow, COLOR_ORANGE, cellOn);
  //       break;
  //   case 2:
  //       setLed(sensorCol, sensorRow, COLOR_GREEN, cellOn);
  //       break;
  //   case 3:
  //       setLed(sensorCol, sensorRow, COLOR_BLUE, cellOn);
  //       break;
  //   default:
  //       setLed(sensorCol, sensorRow, COLOR_VIOLET, cellOn);
  //       break;
  // }
}

int mod(int a, int b) {
    return (a % b + b) % b;
}

static boolean didSingleDragLeft() {
  return sensorRow == prevDownSensorRow && sensorCol == prevDownSensorCol - 1;
}

static boolean didSingleDragRight() {
  return sensorRow == prevDownSensorRow && sensorCol == prevDownSensorCol + 1;
}