// -------------------------------
// TODO:
// -  Handle split-screen
// -------------------------------
boolean isTranspose2Enabled() { return true; }

short curNumCellsTouched = 0;

boolean isDragging = false;
unsigned long dragUpdateTime = 0;
unsigned long lastReleaseAllTime = 0;
short dragFromCol = 0;
short dragFromRow = 0;
short dragFromTransposeLights = 0;
short dragToCol = 0;
short dragToRow = 0;
short dragDeltaCol = 0;
short dragDeltaRow = 0;

byte prevTouchSensorCol = -2;
byte prevTouchSensorRow = -2;
byte prevReleaseSensorCol = -2;
byte prevReleaseSensorRow = -2;


// signed char transposeOctave;            // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
// signed char transposePitch;             // transpose output midi notes. Range is -12 to +12
// signed char transposeLights;            // transpose lights on display. Range is -12 to +12

void transpose2PaintOctaveTransposeDisplay() {
  blinkAllRootNotes = true;
  paintNormalDisplay();
  blinkAllRootNotes = false;
}

void transpose2HandleOctaveTransposeNewTouch() {
  maybeTimeoutDrag();
  curNumCellsTouched++; 
  colorLastCellTouched();

  if (!isDragging) {
    dragStart();
  } else {
    dragUpdate();
  }
  if (dragDeltaRow == 0 && dragDeltaCol != 0) {
    // Split[Global.currentPerSplit].transposePitch++;
    Split[Global.currentPerSplit].transposeLights =
      dragFromTransposeLights + dragDeltaCol;
    normalizeTranspose();
  }

  // Save cell for next event
  prevTouchSensorCol = sensorCol;
  prevTouchSensorRow = sensorRow;

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
  curNumCellsTouched--;

  // Save cell for next event
  prevReleaseSensorCol = sensorCol;
  prevReleaseSensorRow = sensorRow;

  if (curNumCellsTouched == 0) {
    lastReleaseAllTime = micros();
  }
}

static int mod(int a, int b) {
    return (a % b + b) % b;
}

const unsigned long DRAG_TIMEOUT_US = 50000;

static boolean maybeTimeoutDrag() {
  unsigned long now = micros();
  if (curNumCellsTouched == 0 &&
      calcTimeDelta(now, lastReleaseAllTime) >= DRAG_TIMEOUT_US) {
    isDragging = false;
  }
}

static void dragStart() {
  isDragging = true;
  dragFromCol = sensorCol;
  dragFromRow = sensorRow;
  dragFromTransposeLights = Split[Global.currentPerSplit].transposeLights;
  dragUpdate();
}

static void dragUpdate() {
  dragUpdateTime = micros();
  dragToCol = sensorCol;
  dragToRow = sensorRow;
  dragDeltaCol = sensorCol - dragFromCol;
  dragDeltaRow = sensorRow - dragFromRow;
}

static boolean didSingleDragLeft() {
  return sensorRow == prevTouchSensorRow && sensorCol == prevTouchSensorCol - 1;
}

static boolean didSingleDragRight() {
  return sensorRow == prevTouchSensorRow && sensorCol == prevTouchSensorCol + 1;
}

void colorLastCellTouched() {
  // switch (curNumCellsTouched) {
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