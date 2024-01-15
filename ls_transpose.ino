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

static short curNumCellsTouched = 0;

static byte previewNote = 0;
static boolean isDraggingFromRoot = false;
static boolean isDragging = false;
static Layer dragLayer = layerColor;
static unsigned long dragUpdateTime = 0;
static unsigned long lastReleaseAllTime = 0;
static short dragFromCol = 0;
static short dragFromRow = 0;
static short uncommittedMoveOffset = 0;
static short uncommittedPitchOffset = 0;
static short uncommittedColorOffset = 0;
static short dragToCol = 0;
static short dragToRow = 0;
static short dragDeltaCol = 0;
static short dragDeltaRow = 0;
static byte dragFromNote = 0;
static byte midiChannel = 0;

static byte prevTouchSensorCol = -2;
static byte prevTouchSensorRow = -2;
static byte prevReleaseSensorCol = -2;
static byte prevReleaseSensorRow = -2;

// signed char transposeOctave;            // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
// signed char transposePitch;             // transpose output midi notes. Range is -12 to +12
// signed char transposeLights;            // transpose lights on display. Range is -12 to +12

void paintTranspose2Display() {
  // Push commit state
  short prevCommittedColorOffset = getCommittedColorOffset();
  short prevCommittedPitchOffset = getCommittedPitchOffset();
  short prevCommittedMoveOffset = getCommittedMoveOffset();

  // Preview commit
  commitColorOffset(uncommittedColorOffset);
  commitPitchOffset(uncommittedPitchOffset);
  commitMoveOffset(uncommittedMoveOffset);
  
  // Draw main screen
  blinkAllRootNotes = true;
  paintNormalDisplay();
  blinkAllRootNotes = false;

  // Draw popup
  if (isDragging) {
    drawPopup();
  }

  // Pop commit state
  commitColorOffset(prevCommittedColorOffset);
  commitPitchOffset(prevCommittedPitchOffset);
  commitMoveOffset(prevCommittedMoveOffset);
}

static inline short dragOffset() {
  return dragDeltaCol + 5 * dragDeltaRow;
}

void handleTranspose2NewTouch() {
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

  // Touched main area. Count the touch.
  curNumCellsTouched++; 

  // Handle this as a drag event
  if (!isDragging) {
    dragStart();
  } else {
    dragUpdate();
  }

  if (isDraggingFromRoot && (dragLayer == layerOctave || dragLayer == layerPitch)) {
    midiChannel = takeChannel(Global.currentPerSplit, sensorRow);
    previewNote = transposedNote(Global.currentPerSplit, sensorCol, sensorRow);
    midiSendNoteOff(Global.currentPerSplit, previewNote, midiChannel);
    midiSendNoteOn(Global.currentPerSplit, previewNote, 96, midiChannel);

    uncommittedPitchOffset = getCommittedPitchOffset() + dragOffset();
  } else if (isDraggingFromRoot && dragLayer == layerColor) {
    uncommittedColorOffset = mod(getCommittedColorOffset() + dragOffset(), 12);
  } else if (!isDraggingFromRoot){
    uncommittedMoveOffset = getCommittedMoveOffset() + dragOffset();
  }

  // Save cell for next event
  prevTouchSensorCol = sensorCol;
  prevTouchSensorRow = sensorRow;

  updateDisplay();
}

void handleTranspose2Release() {
  // Released touch from popup? Ignore it.
  if (isDragging && sensorCol == 1 && sensorRow <= 4) {
    return;
  }

  // Otherwise, count the release
  curNumCellsTouched--;
  if (curNumCellsTouched == 0) {
    lastReleaseAllTime = micros();
  }

  // We'll respond to the release asynchronously in maybeTimeoutDrag 

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
  // Prevent recursive runs
  static boolean alreadyRunning = false;
  if (alreadyRunning) {
    return false;
  }
  alreadyRunning = true;

  // Detect if timed out
  boolean timedOut = isDragging && curNumCellsTouched == 0 &&
      calcTimeDelta(micros(), lastReleaseAllTime) >= DRAG_TIMEOUT_US;

  // If timed out, we can now handle touch release
  if (timedOut) {
    dragStop();
  }

  alreadyRunning = false;
  return timedOut;
}

static void dragStop() {
  if (!isDragging) {
    return;
  }
  isDragging = false;
  if (isDraggingFromRoot && (dragLayer == layerOctave || dragLayer == layerPitch)) {
    midiSendNoteOff(Global.currentPerSplit, previewNote, midiChannel);
    releaseChannel(Global.currentPerSplit, midiChannel);

    commitPitchOffset(uncommittedPitchOffset);
  } else if (isDraggingFromRoot && dragLayer == layerColor) {
    commitColorOffset(uncommittedColorOffset);
  } else if (!isDraggingFromRoot) {
    commitMoveOffset(uncommittedMoveOffset);
  }
}

static void dragStart() {
  isDragging = true;
  dragFromCol = sensorCol;
  dragFromRow = sensorRow;
  dragFromNote = getPitch(sensorCol, sensorRow);
  isDraggingFromRoot = (dragFromNote % 12) == 0;
  uncommittedMoveOffset = getCommittedMoveOffset();
  uncommittedPitchOffset = getCommittedPitchOffset();
  uncommittedColorOffset = getCommittedColorOffset();
  dragUpdate();
}

static void dragUpdate() {
  dragUpdateTime = micros();
  dragToCol = sensorCol;
  dragToRow = sensorRow;
  dragDeltaCol = sensorCol - dragFromCol;
  dragDeltaRow = sensorRow - dragFromRow;
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
      // byte curNoteColor = Global.paletteColors[Global.activePalette][curNote];
      byte curNoteColor = scaleGetEffectiveNoteColor(curNote);
      boolean curNoteColorIsRoot = scaleGetEffectiveColorOffset() == curNote;
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

static byte getPitch(byte col, byte row) {
  short displayedNote = getNoteNumber(Global.currentPerSplit, col, row) +
    Split[Global.currentPerSplit].transposeOctave;
  return displayedNote;
}

static short getCommittedPitchOffset() {
  return Split[Global.currentPerSplit].transposePitch;
}

static void commitPitchOffset(short pitchOffset) {
  Split[Global.currentPerSplit].transposePitch = pitchOffset;
}

static short getCommittedColorOffset() {
  // TODO: Deal with unassigned scenario
  return ((short)scaleGetEffectiveColorOffset());
}

static inline void commitColorOffset(short colorOffset) {
  scaleSetAssignedColorOffset(mod(colorOffset, 12));
}

static inline short getCommittedMoveOffset() {
  return Split[Global.currentPerSplit].transposeLights;
}

static inline void commitMoveOffset(short moveOffset) {
  Split[Global.currentPerSplit].transposeLights = moveOffset;
}