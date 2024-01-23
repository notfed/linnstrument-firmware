// -------------------------------
// TODO:
// -  Handle split-screen
// -------------------------------
boolean isTranspose2Enabled() { return true; }

enum Layer {
  layerOctave = 0b000010,
  layerPitch  = 0b000100,
  layerColor  = 0b001000,
  layerMode   = 0b010000,
  layerMove   = 0b000001,
  layerScale  = 0b100000,
};

static short curNumCellsTouched = 0;

static byte previewNote = 0;
// static boolean isDraggingFromRoot = false;
static boolean isDragging = false;
static Layer dragLayer = layerColor;
static unsigned long dragUpdateTime = 0;
static unsigned long lastReleaseAllTime = 0;
static short dragFromCol = 0;
static short dragFromRow = 0;
static short uncommittedMoveOffset = 0;
static short uncommittedPitchOffset = 0;
static short uncommittedColorOffset = 0;
static short uncommittedMode = 0;
static short dragToCol = 0;
static short dragToRow = 0;
static short dragDeltaCol = 0;
static short dragDeltaRow = 0;
static byte dragFromNote = 0;
static byte midiChannel = 0;

// signed char transposeOctave;            // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
// signed char transposePitch;             // transpose output midi notes. Range is -12 to +12
// signed char transposeLights;            // transpose lights on display. Range is -12 to +12

void transpose2Reset() {
  isDragging = false;
}

void paintTranspose2Display() {
  // TODO: TEMPORARY: Hardcoding octave and pitch to 0 for now
  Split[Global.currentPerSplit].transposePitch = 0;
  Split[Global.currentPerSplit].transposeOctave = 0;

  // Push commit state
  short prevCommittedPitchOffset = getCommittedPitchOffset();
  short prevCommittedColorOffset = getCommittedColorOffset();
  short prevCommittedMode = getCommittedMode();
  short prevCommittedMoveOffset = getCommittedMoveOffset();

  // Preview commit
  commitPitchOffset(uncommittedPitchOffset);
  commitColorOffset(uncommittedColorOffset);
  commitMode(uncommittedMode);
  commitMoveOffset(uncommittedMoveOffset);
  
  // Draw main screen
  blinkAllRootNotes = true;
  paintNormalDisplay();
  blinkAllRootNotes = false;

  // Draw popup
  drawPopup();

  // Pop commit state
  commitPitchOffset(prevCommittedPitchOffset);
  commitColorOffset(prevCommittedColorOffset);
  commitMode(prevCommittedMode);
  commitMoveOffset(prevCommittedMoveOffset);
}

static inline short dragOffset() {
  return dragDeltaCol + 5 * dragDeltaRow;
}

void handleTranspose2NewTouch() {
  maybeTimeoutDrag();

  // Touched popup? Just change the popup layer and redraw.
  if (sensorCol == 1 && sensorRow <= 4) {
    if (sensorRow == 4) {
      dragLayer = layerOctave;
    } else if (sensorRow == 3) {
      dragLayer = layerPitch;
    } else if (sensorRow == 2) {
      dragLayer = layerColor;
    } else if (sensorRow == 1) {
      dragLayer = layerMode;
    } else if (sensorRow == 0) {
      dragLayer = layerMove;
    }
    updateDisplay();
    return;
  }
  if (sensorCol <= 5 && sensorRow <= 4) {
    return;
  }

  // Touched main area. Count the touch.
  curNumCellsTouched++; 

  // Handle this as a drag event
  if (!isDragging) {
    dragStart();
    // Start playing note
    midiChannel = takeChannel(Global.currentPerSplit, sensorRow); // TODO: Handle channel better
    previewNote = transposedNote(Global.currentPerSplit, sensorCol, sensorRow);
    midiSendNoteOff(Global.currentPerSplit, previewNote, midiChannel);
    midiSendNoteOn(Global.currentPerSplit, previewNote, 96, midiChannel);
  } else {
    dragUpdate();
  }

  // Perform layer-specific drag update
  if (dragLayer == layerOctave || dragLayer == layerPitch) {
    // TODO: Disabled pitch offset for now
    // uncommittedPitchOffset = getCommittedPitchOffset() + dragOffset();
  } else if (dragLayer == layerColor) {
    uncommittedColorOffset = mod(getCommittedColorOffset() - dragOffset(), 12);
  } else if (dragLayer == layerMode) {
    byte maybeUncommittedMode = mod(getCommittedMode() - dragOffset(), 12);
    byte rootNote = 0;
    boolean isRootInScale =
      scaleGetEffectiveScale(Global.activeNotes, maybeUncommittedMode) & (1 << rootNote);
    if (isRootInScale) {
      uncommittedMode = maybeUncommittedMode;
    }
  } else if (dragLayer == layerMove) {
    uncommittedMoveOffset = getCommittedMoveOffset() + dragOffset();
  }

  updateDisplay();
}

void handleTranspose2Release() {
  // Released touch from popup? Ignore it.
  if (!isDragging) {
    return;
  }

  // Otherwise, count the release
  curNumCellsTouched--;
  if (curNumCellsTouched == 0) {
    lastReleaseAllTime = micros();
  }

  // We'll respond to the release asynchronously in maybeTimeoutDrag 
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
  // Stop playing note
  midiSendNoteOff(Global.currentPerSplit, previewNote, midiChannel);
  releaseChannel(Global.currentPerSplit, midiChannel);

  // Handle layer-specific drag stop
  if (dragLayer == layerOctave || dragLayer == layerPitch) {
    commitPitchOffset(uncommittedPitchOffset);
  } else if (dragLayer == layerColor) {
    commitColorOffset(uncommittedColorOffset);
  } else if (dragLayer == layerMode) {
    commitMode(uncommittedMode);
  } else if (dragLayer == layerMove) {
    commitMoveOffset(uncommittedMoveOffset);
  }
}

static void dragStart() {
  isDragging = true;
  dragFromCol = sensorCol;
  dragFromRow = sensorRow;
  dragFromNote = getPitch(sensorCol, sensorRow);
  // isDraggingFromRoot = (dragFromNote % 12) == 0;
  uncommittedPitchOffset = getCommittedPitchOffset();
  uncommittedColorOffset = getCommittedColorOffset();
  uncommittedMode = getCommittedMode();
  uncommittedMoveOffset = getCommittedMoveOffset();
  dragUpdate();
}

static void dragUpdate() {
  dragUpdateTime = micros();
  dragToCol = sensorCol;
  dragToRow = sensorRow;
  dragDeltaCol = sensorCol - dragFromCol;
  dragDeltaRow = sensorRow - dragFromRow;
}

static void drawPopup() {
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
    } else if (row == 0 && dragLayer == layerMove) {
      setLed(col, row, COLOR_WHITE, cellFastPulse);
    } else {
      setLed(col, row, COLOR_WHITE, cellOn);
    }
  }
  // Draw notes
  // Show notes (1) only if in scale, (2) at correct pitch, (3) with correct color, and (4) with pulsing root
  for (byte row = 0; row <= 3; ++row) {
    for (byte col = 2; col <= 4; ++col) {
      byte curNote = noteAtCell(col, row);
      byte curPitch = mod(curNote + getCommittedPitchOffset(), 12);
      byte curNoteIsRoot = isRootNote(curNote);
      byte curPitchIsRoot = isRootNote(curPitch);
      boolean curNoteIsModeOffset = scaleGetEffectiveMode() == curNote;
      boolean curCellIsOctave = ((getCommittedPitchOffset() + 60) / 12) == curNote;
      boolean curNoteIsInScale = scaleContainsNote(curNote);
      byte curNoteColor = curNoteIsInScale ? scaleGetEffectiveNoteColor(curNote) : COLOR_WHITE;
      
      if (dragLayer == layerOctave && curCellIsOctave) {
        setLed(col, row, COLOR_WHITE, cellOn);
      } else if (dragLayer == layerColor && (curNoteIsRoot || curNoteIsInScale)) {
        setLed(col, row, curNoteColor, curNoteIsRoot ? cellSlowPulse : cellOn);
      } else if (dragLayer == layerPitch && (curPitchIsRoot || curNoteIsInScale)) {
        setLed(col, row, curNoteColor, curPitchIsRoot ? cellSlowPulse : cellOn); // ?
      } else if (dragLayer == layerMode && (curNoteIsModeOffset || curNoteIsInScale)) {
        setLed(col, row, curNoteColor, curNoteIsModeOffset ? cellSlowPulse : cellOn);
      } else if (dragLayer == layerMove && (curNoteIsRoot || curNoteIsInScale)) {
        setLed(col, row, curNoteColor, curNoteIsRoot ? cellSlowPulse : cellOn);
      } else {
        setLed(col, row, COLOR_OFF, cellOff);
      }
    }
  }
}

// Thoughts:
// - Seems we only need layers (1) move, (2) color, (3) mode

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

static inline short getCommittedMode() {
  // TODO: Deal with unassigned scenario
  return scaleGetEffectiveMode();
}

static inline void commitMode(short mode) {
  scaleSetAssignedMode(mode);
}

static inline short getCommittedMoveOffset() {
  return Split[Global.currentPerSplit].transposeLights;
}

static inline void commitMoveOffset(short moveOffset) {
  Split[Global.currentPerSplit].transposeLights = moveOffset;
}

static inline boolean isRootNote(short note) {
  return mod(note + getCommittedPitchOffset(),  12) == 0;
}