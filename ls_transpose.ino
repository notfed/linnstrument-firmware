// -------------------------------
// TODO:
// -  Handle split-screen
// -------------------------------
boolean isTranspose2Enabled() { return true; }

enum Layer {
  layerAll    = 0b01110,
  layerOctave = 0b00001,
  layerPitch  = 0b00010,
  layerColor  = 0b00100,
  layerMode   = 0b01000,
  layerScale  = 0b10000,
};

short curNumCellsTouched = 0;

byte previewNote = 0;
boolean isDraggingFromRoot = false;
boolean isDragging = false;
Layer dragLayer = layerColor;
unsigned long dragUpdateTime = 0;
unsigned long lastReleaseAllTime = 0;
short dragFromCol = 0;
short dragFromRow = 0;
short transposePitchAtDragStart = 0;
short transposeLightsAtDragStart = 0;
short paletteColorOffsetAtDragStart = 0;
short dragToCol = 0;
short dragToRow = 0;
short dragDeltaCol = 0;
short dragDeltaRow = 0;
byte dragFromNote = 0;
byte midiChannel = 0;

byte prevTouchSensorCol = -2;
byte prevTouchSensorRow = -2;
byte prevReleaseSensorCol = -2;
byte prevReleaseSensorRow = -2;

// signed char transposeOctave;            // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
// signed char transposePitch;             // transpose output midi notes. Range is -12 to +12
// signed char transposeLights;            // transpose lights on display. Range is -12 to +12

void transpose2PaintOctaveTransposeDisplay() {
  // Maybe preview color offset
  byte actualAssignedColorOffset = scaleGetAssignedColorOffset();
  if (isDragging && isDraggingFromRoot && dragLayer == layerColor) {
    byte previewAssignedColorOffset = mod(paletteColorOffsetAtDragStart + dragOffset(), 12);
    scaleSetAssignedColorOffset(previewAssignedColorOffset);
  }

  // Draw main screen
  blinkAllRootNotes = true;
  paintNormalDisplay();
  blinkAllRootNotes = false;

  // ???
  blinkNote = -1; // TODO: Do we use this?

  // Draw popup
  if (isDragging) {
    drawPopup();
  }

  // Restore color offset
  if (isDragging && isDraggingFromRoot && dragLayer == layerColor) {
    scaleSetAssignedColorOffset(actualAssignedColorOffset);
  }
}

static inline short dragOffset() {
  return dragDeltaCol + 5 * dragDeltaRow;
}

void transpose2HandleOctaveTransposeNewTouch() {
  maybeTimeoutDrag();

  // Touched popup? Just change the popup layer and redraw.
  if (isDragging && sensorCol == 1 && sensorRow <= 4) {
    if (sensorRow == 4) {
      dragLayer = layerOctave;
    } else if (sensorRow == 3) {
      dragLayer = layerPitch;
    } else if (sensorRow == 2) {
      dragLayer = layerColor;
    } else if (sensorRow == 1) {
      dragLayer = layerMode;
    } else if (sensorRow == 0) {
      dragLayer = layerAll;
    }
    updateDisplay();
    return;
  }

  // Otherwise, handle dragging
  curNumCellsTouched++; 

  if (!isDragging) {
    dragStart();
  } else {
    dragUpdate();
  }

  if (isDraggingFromRoot && dragLayer == layerColor) {
    // scaleSetAssignedColorOffset(mod(paletteColorOffsetAtDragStart + dragOffset(), 12));
  } else if (isDraggingFromRoot && (dragLayer == layerOctave || dragLayer == layerPitch)) {
    midiChannel = takeChannel(Global.currentPerSplit, sensorRow); //takeChannel(Global.currentPerSplit, sensorRow);
    previewNote = transposedNote(Global.currentPerSplit, sensorCol, sensorRow);
    midiSendNoteOff(Global.currentPerSplit, previewNote, midiChannel);
    midiSendNoteOn(Global.currentPerSplit, previewNote, 96, midiChannel);
  } else if (!isDraggingFromRoot){
    Split[Global.currentPerSplit].transposeLights =
      transposeLightsAtDragStart + dragOffset();
    setLed(25, 0, COLOR_BLUE, cellOn); // DEBUG. REMOVE!
  }
  // normalizeTranspose();

  // Save cell for next event
  prevTouchSensorCol = sensorCol;
  prevTouchSensorRow = sensorRow;

  updateDisplay();
}

void transpose2HandleOctaveTransposeRelease() {
  // Released touch from popup? Don't treat it like other releases.
  if (isDragging && sensorCol == 1 && sensorRow <= 4) {
    return;
  }

  curNumCellsTouched--;

  if (curNumCellsTouched == 0) {
    lastReleaseAllTime = micros();
  }

  // Save cell for next event
  prevReleaseSensorCol = sensorCol;
  prevReleaseSensorRow = sensorRow;
}

static short mod(short a, short b) {
  return (a % b + b) % b;
}

const unsigned long DRAG_TIMEOUT_US = 50000;

// This is called asynchronously, often. Return true if timeout occurred.
boolean maybeTimeoutDrag() {
  unsigned long now = micros();
  if (isDragging && curNumCellsTouched == 0 &&
      calcTimeDelta(now, lastReleaseAllTime) >= DRAG_TIMEOUT_US) {
    dragStop();
    return true;
  }
  return false;
}

static void dragStop() {
  if (!isDragging) {
    return;
  }
  isDragging = false;
  if (isDraggingFromRoot && dragLayer == layerPitch) {
    midiSendNoteOff(Global.currentPerSplit, previewNote, midiChannel);
    releaseChannel(Global.currentPerSplit, midiChannel);
    Split[Global.currentPerSplit].transposePitch =
      transposePitchAtDragStart + dragOffset();
  } else if (isDraggingFromRoot && dragLayer == layerColor) {
    scaleSetAssignedColorOffset(mod(paletteColorOffsetAtDragStart + dragOffset(), 12));
  }
}

static void dragStart() {
  isDragging = true;
  dragFromCol = sensorCol;
  dragFromRow = sensorRow;
  dragFromNote = getPitch(sensorCol, sensorRow);
  isDraggingFromRoot = (dragFromNote % 12) == 0;
  // if (isDraggingFromRoot) {
    // blinkMiddleRootNote = true;
    // paintNormalDisplay();
    // setLed(sensorCol, sensorRow, COLOR_RED, cellFastPulse); // scaleGetEffectiveNoteColor(dragFromNote)
    // blinkMiddleRootNote = false;
  // }
  transposePitchAtDragStart = Split[Global.currentPerSplit].transposePitch;
  transposeLightsAtDragStart = Split[Global.currentPerSplit].transposeLights;
  paletteColorOffsetAtDragStart = ((short)scaleGetEffectiveColorOffset());
  // if (isDraggingFromRoot) {
  //   dragLayer = layerColor;
  // } else {
  //   dragLayer = layerAll;
  // }
  dragUpdate();
}

static void dragUpdate() {
  dragUpdateTime = micros();
  dragToCol = sensorCol;
  dragToRow = sensorRow;
  dragDeltaCol = sensorCol - dragFromCol;
  dragDeltaRow = sensorRow - dragFromRow;
}

static byte getPitch(byte col, byte row) {
  short displayedNote = getNoteNumber(Global.currentPerSplit, col, row) +
    Split[Global.currentPerSplit].transposeOctave;
  return displayedNote;
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

void drawPopup() {
  // Draw right border
  for (int row = 0; row <= 4; row++) {
    int col = 5;
    setLed(col, row, COLOR_WHITE, cellOn);
  }
  // Draw top border
  for (int col = 4; col >= 2; col--) {
    int row = 4;
    setLed(col, row, COLOR_WHITE, cellOn);
  }
  // Draw left border
  for (int row = 4; row >= 0; row--) {
    int col = 1;
    if (row == 4 && dragLayer == layerOctave) {
      setLed(col, row, COLOR_WHITE, cellFastPulse);
    } else if (row == 3 && dragLayer == layerPitch) {
      setLed(col, row, COLOR_WHITE, cellFastPulse);
    } else if (row == 2 && dragLayer == layerColor) {
      setLed(col, row, COLOR_WHITE, cellFastPulse);
    } else if (row == 1 && dragLayer == layerMode) {
      setLed(col, row, COLOR_WHITE, cellFastPulse);
    } else {
      setLed(col, row, COLOR_WHITE, cellOn);
    }
  }
  // Draw notes
  for (byte row = 0; row <= 3; ++row) {
    for (byte col = 2; col <= 4; ++col) {
      byte curNote = noteAtCell(col, row);
      byte curNoteColor = Global.paletteColors[Global.activePalette][curNote];
      boolean curNoteColorIsRoot = scaleGetAssignedColorOffset() == curNote;
      // byte mode = scaleGetAssignedMode();

      // if (dragLayer == layerOctave) {
      //     setLed(col, row, COLOR_OFF, cellOff);
      // } else if (dragLayer == layerPitch) {
      //     setLed(col, row, COLOR_OFF, cellOff);
      // } else if (dragLayer == layerColor) {
      //     setLed(col, row, curNoteColor, curNoteColorIsRoot ? cellSlowPulse : cellOn);
      // } else {
        if (scaleContainsNote(curNote)) {
           setLed(col, row, curNoteColor, curNoteColorIsRoot ? cellSlowPulse : cellOn);
        } else {
           setLed(col, row, COLOR_OFF, cellOff);
        }
      // }
    }
  }
}
