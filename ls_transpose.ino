// -------------------------------
// TODO:
// -  Handle split-screen
// -------------------------------

// TODO: Refactor to these methods:
//
//
// onPressDown, onPressUp, onShortPress, onLongPress, onDragStart, onDragUpdate, onDragEnd
//
// onNewTouchTransposeMenu(byte row, byte col)
// onTapTransposeMenu(byte row, byte col)
// onHoldTransposeMenu(byte row, byte col)
// onDragTransposeMenu(byte row, byte col)
// onReleaseTransposeMenu(byte row, byte col)
//
// onTapOptionExpander(TransposeOption option)
// onHoldOptionExpander(TransposeOption option)
// onReleaseOptionExpander(TransposeOption option)
//
// setOptionExpanded(TransposeOption option)
// getOptionExpanded(TransposeOption option)
//
// onTapOptionCell(TransposeOption option, byte index)
// onHoldOptionCell(TransposeOption option, byte index)
// onDragOptionCell(TransposeOption option, byte index)
// onReleaseOptionCell(TransposeOption option, byte index)

boolean isTranspose2Enabled() { return Global.colorScalesEnabled; }

enum Layer {
  layerOctave = 0b000010,
  layerPitch  = 0b000100,
  layerColor  = 0b001000,
  layerMode   = 0b010000,
  layerMove   = 0b000001,
  layerScale  = 0b100000,
};

static int curVisibleOptions = OPTION_PITCH | OPTION_SCALE | OPTION_COLOR;
static int curEditOption = OPTION_NONE;
static byte brushColor = COLOR_RED;

static short curNumCellsTouched = 0;

static byte previewNote = 0;
// static boolean isDraggingFromRoot = false;
static boolean isDragging = false;
static Layer dragLayer = layerMove;
static unsigned long dragUpdateTime = 0;
static unsigned long lastReleaseAllTime = 0;
static uint32_t lastTouchTime = 0;
static unsigned long lastTouchCol = 0;
static unsigned long lastTouchRow = 0;
static short dragFromCol = 0;
static short dragFromRow = 0;
static short uncommittedMoveOffset = 0;
static short uncommittedPitchOffset = 0;
static short uncommittedColorOffset = 0;

// Layer-related temporary variables
static short uncommittedMode = 0;
static short dragFromMode = 0;
static short rawUncommittedMode = 0;

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

// When dragging the mode layer, this is called to paint a cell
// From the dragged row, just color one octave, starting at the mode offset. Hide other notes.
struct ColorAndDisplayOverride transpose2NoteFilter(byte split, byte col, byte row) {
  // If not dragging this row, paint this row white
  if (row != dragFromRow) {
     return (struct ColorAndDisplayOverride) { true, COLOR_WHITE, false, cellOff };
  }

  short dragFromNote = getNoteNumber(split, dragFromCol, dragFromRow) + Split[split].transposeOctave;
  short curNote = getNoteNumber(split, col, row) + Split[split].transposeOctave;
  short dragToMode = rawUncommittedMode;
  short rootNote = ((dragFromNote + dragFromMode) / 12) * 12 - dragToMode;

  if (curNote >= rootNote && curNote < (rootNote + 12)) {
     return (struct ColorAndDisplayOverride) { false, 0, false, cellOff };
  } else {
     return (struct ColorAndDisplayOverride) { true, COLOR_BLACK, true, cellOff };
  }
}

void paintTranspose2Display() {
  // // TODO: TEMPORARY: Hardcoding octave and pitch to 0 for now
  // Split[Global.currentPerSplit].transposePitch = 0;
  // Split[Global.currentPerSplit].transposeOctave = 0;

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
  if (isDragging && dragLayer == layerMode) {
    displayNoteFilter = &transpose2NoteFilter;
  }
  blinkAllRootNotes = true;
  paintNormalDisplay();
  blinkAllRootNotes = false;
  displayNoteFilter = NULL;

  // Draw transpose menu
  drawTransposeMenu();

  // Pop commit state
  commitPitchOffset(prevCommittedPitchOffset);
  commitColorOffset(prevCommittedColorOffset);
  commitMode(prevCommittedMode);
  commitMoveOffset(prevCommittedMoveOffset);
}

static inline short dragOffset() {
  return dragDeltaCol + 5 * dragDeltaRow;
}

static boolean isInsideTransposeMenu(byte row, byte col) {
  return
    (sensorRow >= TR_ROW_PITCH && sensorRow <= TR_ROW_BANK && sensorCol <= 14) &&
    (sensorCol <= 2 || 
      (sensorRow == TR_ROW_PITCH && (OPTION_PITCH & curVisibleOptions)) ||
      (sensorRow == TR_ROW_SCALE && (OPTION_SCALE & curVisibleOptions)) || 
      (sensorRow == TR_ROW_COLOR && (OPTION_COLOR & curVisibleOptions)) ||
      (sensorRow == TR_ROW_BANK  && (OPTION_BANK  & curVisibleOptions))
    );
}

// Touching a cell within the transpose menu?
// - In normal mode: modify the layer.
// - In edit mode: set the layer's offset.
static void onTapOptionCell(TransposeOption option, byte index) {
  switch(option) {
    case OPTION_PITCH:
      if (curEditOption == OPTION_PITCH) {
      } else {
        commitPitchOffset(index);
      }
      break;
    case OPTION_SCALE:
      if (curEditOption == OPTION_SCALE) {
        Global.mainNotes[Global.activeNotes] ^= (1 << index);
      } else {
        commitMode(index);
      }
      break;
    case OPTION_COLOR:
      if (curEditOption == OPTION_COLOR) {
        if (Global.paletteColors[Global.activePalette][index] == brushColor) {
          Global.paletteColors[Global.activePalette][index] = COLOR_OFF;
        } else {
          Global.paletteColors[Global.activePalette][index] = brushColor;
        }
      } else {
        commitColorOffset(index);
      }
      break;
    case OPTION_BANK:
      byte colorPaletteId = index;
      byte scaleId = index;
      if (curEditOption == OPTION_BANK) {
        // Copy the current active color palette into the chosen palette.
        for (byte i = 0; i < 12; i++) {
          Global.paletteColors[colorPaletteId][i] = Global.paletteColors[Global.activePalette][i];
        }
        // Copy the current active scale into the chosen scale.
        Global.mainNotes[scaleId] = Global.mainNotes[Global.activeNotes];
      } else {
        Global.activeNotes = scaleId;
        Global.activePalette = colorPaletteId;
      }
      break;
    }
}

void handleTranspose2NewTouch() {
  maybeTimeoutDrag();
  lastTouchTime = micros();
  lastTouchCol = sensorCol;
  lastTouchRow = sensorRow;

  if (isInsideTransposeMenu(sensorRow, sensorCol)) {
      // Touching a menu option cell?
      if (sensorCol >= 2 && sensorCol <= 13) {
        byte index = sensorCol - 2;
        switch(sensorRow) {
          case TR_ROW_PITCH: onTapOptionCell(OPTION_PITCH, index); break;
          case TR_ROW_SCALE: onTapOptionCell(OPTION_SCALE, index); break;
          case TR_ROW_COLOR: onTapOptionCell(OPTION_COLOR, index); break;
          case TR_ROW_BANK:  onTapOptionCell(OPTION_BANK,  index); break;
        }
        updateDisplay();
        return;
      }
      // Touching the left or right white border? Handle on release.
      else {
        return;
      }
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
      rawUncommittedMode = getCommittedMode() - dragOffset();
    }
  } else if (dragLayer == layerMove) {
    uncommittedMoveOffset = getCommittedMoveOffset() + dragOffset();
  }

  updateDisplay();
}

// Transform the color palette into a circle of fifths by rotating every other color by a fifth
static void permuteColorFifths() {
  for (byte note = 1; note <= 5; note+=2) {
    byte curColor = Global.paletteColors[Global.activePalette][note];
    byte fifthColor = Global.paletteColors[Global.activePalette][(note+6)%12];
    Global.paletteColors[Global.activePalette][(note+6)%12] = curColor;
    Global.paletteColors[Global.activePalette][note] = fifthColor;
  }
}

// Tapped an option expander (white, left-side cell)? Toggle it and take it out of edit mode.
static void onTapOptionExpander(TransposeOption option) {
  switch(sensorRow) {
    case TR_ROW_PITCH: curVisibleOptions ^= OPTION_PITCH; curEditOption &= ~OPTION_PITCH; break;
    case TR_ROW_SCALE: curVisibleOptions ^= OPTION_SCALE; curEditOption &= ~OPTION_SCALE; break;
    case TR_ROW_COLOR:
      if (curEditOption == OPTION_COLOR) {
        brushColor = colorCycle(brushColor, false);
      } else {
        curVisibleOptions ^= OPTION_COLOR;
        curEditOption &= ~OPTION_COLOR;
      }
      break;
    case TR_ROW_BANK: curVisibleOptions ^= OPTION_BANK; curEditOption &= ~OPTION_BANK; break;
  }
}

// Long held, then released, an option expander (white, left-side cell)? Enable it and put it in edit mode.
static void onReleaseOptionExpander(TransposeOption option) {
  switch(sensorRow) {
    case TR_ROW_PITCH: curVisibleOptions |= OPTION_PITCH; curEditOption = OPTION_PITCH; break;
    case TR_ROW_SCALE: curVisibleOptions |= OPTION_SCALE; curEditOption = OPTION_SCALE; break;
    case TR_ROW_COLOR:
      if (curEditOption == OPTION_COLOR) {
        permuteColorFifths();
      } else { 
        brushColor = scaleGetEffectiveNoteColor(0);
        curVisibleOptions |= OPTION_COLOR;
        curEditOption = OPTION_COLOR;
      }
      break;
    case TR_ROW_BANK: curVisibleOptions |= OPTION_BANK; curEditOption = OPTION_BANK; break;
  }
}

void handleTranspose2Release() {
  boolean touchWasInsideTransposeMenu = isInsideTransposeMenu(lastTouchRow, lastTouchCol);
  boolean touchedAndReleasedSameCell = lastTouchCol == sensorCol && lastTouchRow == sensorRow;

  // Exit edit mode
  if (touchWasInsideTransposeMenu && curEditOption != OPTION_NONE &&
    (sensorRow == TR_ROW_PITCH && !(curEditOption == OPTION_PITCH) ||
     sensorRow == TR_ROW_SCALE && !(curEditOption == OPTION_SCALE) ||
     sensorRow == TR_ROW_COLOR && !(curEditOption == OPTION_COLOR) ||
     sensorRow == TR_ROW_BANK  && !(curEditOption == OPTION_BANK))
  ) {
    curEditOption = OPTION_NONE;
    updateDisplay();
    return;
  }

  // Tapped/held an expander
  if (touchWasInsideTransposeMenu && touchedAndReleasedSameCell && sensorCol == 1) {
    boolean wasLongHold = calcTimeDelta(micros(), lastTouchTime) >= TR_HOLD_TIME_US;
    switch(sensorRow) {
      case TR_ROW_PITCH: wasLongHold ? onReleaseOptionExpander(OPTION_PITCH) : onTapOptionExpander(OPTION_PITCH); break;
      case TR_ROW_SCALE: wasLongHold ? onReleaseOptionExpander(OPTION_SCALE) : onTapOptionExpander(OPTION_SCALE); break;
      case TR_ROW_COLOR: wasLongHold ? onReleaseOptionExpander(OPTION_COLOR) : onTapOptionExpander(OPTION_COLOR); break;
      case TR_ROW_BANK:  wasLongHold ? onReleaseOptionExpander(OPTION_BANK ) : onTapOptionExpander(OPTION_BANK ); break;
    }
    updateDisplay();
    return;
  }

  // Release drag
  if (isDragging) {
    curNumCellsTouched--;
    if (curNumCellsTouched == 0) {
      lastReleaseAllTime = micros();
    }
    // We'll respond to the drag release asynchronously in maybeTimeoutDrag 
  }
}

static short mod(short a, short b) {
  return (a % b + b) % b;
}

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
      calcTimeDelta(micros(), lastReleaseAllTime) >= TR_DRAG_TIMEOUT_US;

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
  dragFromMode = uncommittedMode;
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

static void drawTransposeMenu() {
  // Draw cells between rows 0..2, and columns 1..12
  // Row 0 will draw 12 cells, each with the color of the note, and pulsing if it's the pitch offset
  // Row 1 will draw 12 cells, each with the color of the note, and pulsing if it's the mode offset
  // Row 2 will draw 12 cells, each with the color of the note, and pulsing if it's the color offset  
  for (int col = 2; col <= 13; col++) {
    byte curNote = col - 2;
    boolean curNoteIsInScale = scaleContainsNote(curNote);

    byte curNoteColor = curNoteIsInScale ? scaleGetEffectiveNoteColor(curNote) : COLOR_WHITE;

    byte curNoteIsPitchOffset =  mod(getCommittedPitchOffset(), 12) == curNote;
    boolean curNoteIsModeOffset = mod(scaleGetEffectiveMode(), 12) == curNote;
    byte curNoteIsColorOffset = mod(getCommittedColorOffset(), 12) == curNote;
    byte curNoteIsBank = mod(Global.activeNotes, 12) == curNote;
  
    // Pitch
    if (curVisibleOptions & OPTION_PITCH) {
      setLed(col, TR_ROW_PITCH, scaleGetEffectiveNoteColor(0), curNoteIsPitchOffset ? cellOn : cellOff);
    }
    // Scale
    if (curVisibleOptions & OPTION_SCALE) {
      // TODO: COLOR_WHITE or curNoteColor?
      setLed(col, TR_ROW_SCALE, COLOR_WHITE, curNoteIsModeOffset ? cellSlowPulse : (curNoteIsInScale ? cellOn : cellOff));
    }
    // Color
    if (curVisibleOptions & OPTION_COLOR) {
      setLed(col, TR_ROW_COLOR, scaleGetNoteColor(curNote), curNoteIsColorOffset ? cellSlowPulse : cellOn);
    }
    // Bank
    if (curVisibleOptions & OPTION_BANK) {
      setLed(col, TR_ROW_BANK, curNoteIsBank ? COLOR_WHITE : COLOR_OFF, curNoteIsBank ? cellSlowPulse : cellOff);
    }
  }
  // Draw left-side white border
  setLed(1, TR_ROW_PITCH, COLOR_WHITE, curEditOption & OPTION_PITCH ? cellSlowPulse : cellOn);
  setLed(1, TR_ROW_SCALE, COLOR_WHITE, curEditOption & OPTION_SCALE ? cellSlowPulse : cellOn);
  if (curEditOption & OPTION_COLOR) {
    setLed(1, TR_ROW_COLOR, brushColor, cellSlowPulse);
  } else {
    setLed(1, TR_ROW_COLOR, COLOR_WHITE, cellOn);
  }
  setLed(1, TR_ROW_BANK , COLOR_WHITE, curEditOption & OPTION_BANK  ? cellSlowPulse : cellOn);
  // Draw right-side white border
  setLed(curVisibleOptions & OPTION_PITCH ? 14 : 2, TR_ROW_PITCH, COLOR_WHITE, curEditOption & OPTION_PITCH ? cellSlowPulse : cellOn);
  setLed(curVisibleOptions & OPTION_SCALE ? 14 : 2, TR_ROW_SCALE, COLOR_WHITE, curEditOption & OPTION_SCALE ? cellSlowPulse : cellOn);
  setLed(curVisibleOptions & OPTION_COLOR ? 14 : 2, TR_ROW_COLOR, COLOR_WHITE, curEditOption & OPTION_COLOR ? cellSlowPulse : cellOn);
  setLed(curVisibleOptions & OPTION_BANK  ? 14 : 2, TR_ROW_BANK,  COLOR_WHITE, curEditOption & OPTION_BANK  ? cellSlowPulse : cellOn);
}

static void drawPopupOld() {
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
        setLed(col, row, getPrimaryColor(Global.currentPerSplit), curNoteIsModeOffset ? cellSlowPulse : cellOn);
      } else if (dragLayer == layerMove && (curNoteIsRoot || curNoteIsInScale)) {
        setLed(col, row, getPrimaryColor(Global.currentPerSplit), curNoteIsRoot ? cellSlowPulse : cellOn);
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
  uncommittedPitchOffset = pitchOffset;
  Split[Global.currentPerSplit].transposePitch = pitchOffset;
}

static short getCommittedColorOffset() {
  // TODO: Deal with unassigned scenario
  return ((short)scaleGetEffectiveColorOffset());
}

static inline void commitColorOffset(short colorOffset) {
  uncommittedColorOffset = colorOffset;
  scaleSetAssignedColorOffset(mod(colorOffset, 12));
}

static inline short getCommittedMode() {
  // TODO: Deal with unassigned scenario
  return scaleGetEffectiveMode();
}

static inline void commitMode(short mode) {
  uncommittedMode = mode;
  rawUncommittedMode = mode;
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