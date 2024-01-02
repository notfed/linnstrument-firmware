static byte noteAtCell(byte col, byte row) {
  if (!(col >= 2 && col <= 4 && row >= 0 && row <= 3)) {
    return -1;
  }
  return (col - 2) + (row * 3);
}

static void scaleToggleNote(byte scaleId, byte note) {
  if (scaleId > 11 || note > 11) {
    return;
  }
  int* scale = &Global.mainNotes[scaleId];
  *scale ^= 1 << note;
}

static boolean scaleContainsNote(byte scaleId, byte note) {
  if (scaleId > 11 || note > 11) {
    return false;
  }
  int* scale = &Global.mainNotes[scaleId];
  return *scale & 1 << note;
}

static byte scaleGetAssignedColorOffset() {
  return Global.scaleColorOffset - 1;
}

static void scaleSetAssignedColorOffset(byte offset) {
  if (offset > 11) {
    Global.scaleColorOffset = 0;
  } else {
    Global.scaleColorOffset = offset + 1;
  }
}

static byte scaleGetEffectiveColorOffset() {
  if (Global.scaleColorOffset == -1) {
    return 0;
  } else {
    return Global.scaleColorOffset - 1;
  }
}

static byte scaleGetAssignedMode(byte scaleId) {
  if (scaleId > 11) {
    return 0;
  }
  return Global.scaleMode[scaleId] - 1;
}

static byte scaleGetEffectiveMode(byte scaleId) {
  if (scaleId > 11) {
    return 0;
  }
  byte mode = Global.scaleMode[scaleId] - 1;
  if (mode > 11) {
    return 0;
  }
  return Global.scaleMode[scaleId] - 1;
}

static void scaleSetAssignedMode(byte scaleId, byte mode) {
  if (scaleId > 11) {
    return;
  } else if (mode > 11) {
    Global.scaleMode[scaleId] = 0;
  } else {
    Global.scaleMode[scaleId] = mode + 1;
  }
}

static int rotateRight12(int x, int n) {
    n %= 12; 
    x = (x >> n) | (x << (12 - n));
    x &= 0xFFF;
    return x;
}

int scaleGetEffectiveScale() {
  int activeScaleId = Global.activeNotes;
  byte mode = scaleGetEffectiveMode(activeScaleId);
  return rotateRight12(Global.mainNotes[activeScaleId], mode);
}

byte scaleGetEffectiveNoteColor(byte note) {
  if (note > 11) {
    return COLOR_OFF;
  }
  return Global.scaleNoteColors[(note + scaleGetEffectiveColorOffset()) % 12];
}

void scaleRedraw() {
  switch (lightSettings) {
    case LIGHTS_MAIN:
      accentColor = COLOR_BLACK;
      if (!customLedPatternActive) {
        lightLed(1, 0);
        scaleRedrawMain();
      }
      break;
    case LIGHTS_ACCENT:
      if (!customLedPatternActive) {
        setLed(1, 1, accentColor, cellOn);
        scaleRedrawAccent();
      }
      break;
    case LIGHTS_ACTIVE:
      accentColor = COLOR_BLACK;
      lightLed(1, 2);
      scaleRedrawScaleSel();
      break;
  }
}

// [GLOBAL SETTINGS]->[SCALE SEL] :: Show which scale is currently active
static void scaleRedrawScaleSel() {
  byte scaleId = Global.activeNotes;
  for (byte row = 0; row <= 3; ++row) {
    for (byte col = 2; col <= 4; ++col) {
      byte scaleIndex = noteAtCell(col, row);
      if (scaleIndex == Global.activeNotes) {
        lightLed(col, row);
      }
    }
  }
}

// [GLOBAL SETTINGS]->ACCENT :: Draw each note's configured color (of the current scale)
static void scaleRedrawAccent() {
  byte scaleId = Global.activeNotes;
  for (byte row = 0; row <= 3; ++row) {
    for (byte col = 2; col <= 4; ++col) {
      byte note = noteAtCell(col, row);
      byte color = Global.scaleNoteColors[note];
      boolean isColorOffset = scaleGetAssignedColorOffset() == note;
      setLed(col, row, color, isColorOffset ? cellSlowPulse : cellOn);
    }
  }
}

// [GLOBAL SETTINGS]->MAIN :: Draw all notes (of the current scale)
static void scaleRedrawMain() {
  byte scaleId = Global.activeNotes;
  byte mode = scaleGetAssignedMode(scaleId);

  for (byte row = 0; row <= 3; ++row) {
    for (byte col = 2; col <= 4; ++col) {
      byte note = noteAtCell(col, row);
      if (scaleContainsNote(scaleId, note)) {
        if (note == mode) {
          setLed(col, row, globalAltColor, cellOn);
        } else {
          setLed(col, row, globalColor, cellOn);
        }
      }
    }
  }
}

void scaleCellOnTouchStart(byte sensorCol, byte sensorRow) {
  switch (sensorCol) {
    case 1:
      switch (sensorRow) {
        case LIGHTS_ACCENT:
        case LIGHTS_MAIN:
        case LIGHTS_ACTIVE:
          if (!customLedPatternActive) {
            lightSettings = sensorRow;
          }
          break;
      }
      break;
  }
}

// Called after scaleCellOnTouchStart, gives us a chance to start pulsing a holdable cell
void scaleCellOnTouchStartHold(byte sensorCol, byte sensorRow) {
  byte activeScaleId = Global.activeNotes;

  if (sensorCol == 1 && sensorRow == LIGHTS_ACCENT && lightSettings == LIGHTS_ACCENT) {
    setLed(sensorCol, sensorRow, accentColor, cellSlowPulse);
  }
  byte note = noteAtCell(sensorCol, sensorRow);
  if (note > 11) {
    return;
  }
  if (lightSettings == LIGHTS_MAIN) {
    if (scaleContainsNote(activeScaleId, note)) {
      setLed(sensorCol, sensorRow, globalColor, cellSlowPulse);
    }
  } else if (lightSettings == LIGHTS_ACCENT) {
    if (scaleContainsNote(activeScaleId, note)) {
      byte currentColor = Global.scaleNoteColors[note];
      setLed(sensorCol, sensorRow, currentColor, cellSlowPulse);
    }
  } else if (lightSettings == LIGHTS_ACTIVE) {
      setLed(sensorCol, sensorRow, globalColor, cellSlowPulse);
  }
}

void scaleCellOnTouchEnd(byte sensorCol, byte sensorRow) {
  byte activeScaleId = Global.activeNotes;

  if (sensorCol == 1 && sensorRow == LIGHTS_ACCENT && lightSettings == LIGHTS_ACCENT) {
      accentColor = colorCycle(accentColor, false);
      setLed(1, 1, accentColor, cellOn);
  }
  
  byte pressedNote = noteAtCell(sensorCol, sensorRow);
  if (pressedNote>11) {
    return;
  }

  if (lightSettings == LIGHTS_ACTIVE &&
      ensureCellBeforeHoldWait(COLOR_BLACK, cellOn)) {
    Global.activeNotes = pressedNote;
    // TODO: Change how we handle custom led layer
    // loadCustomLedLayer(-1);
  }
  else if (lightSettings == LIGHTS_ACCENT &&
      ensureCellBeforeHoldWait(COLOR_BLACK, cellOn)) {
    if (!customLedPatternActive) { // TODO: Lift these checks to function start
      if (Global.scaleNoteColors[pressedNote] != accentColor) {
        Global.scaleNoteColors[pressedNote] = accentColor;
      } else {
        Global.scaleNoteColors[pressedNote] = COLOR_BLACK;
      }
    }
  }
  else if (lightSettings == LIGHTS_MAIN &&
      ensureCellBeforeHoldWait(COLOR_BLACK, cellOn)) {
    if (!customLedPatternActive) {
      // Toggle this note in the scale.
      scaleToggleNote(activeScaleId, pressedNote);
      // TODO: Clear the mode note if it's not in the scale
      // byte mode = scaleGetAssignedMode(activeScaleId);
      // if (!scaleContainsNote(activeScaleId, mode)) {
      //   scaleSetAssignedMode(activeScaleId, -1);
      // }
    }
  }
}

void scaleCellOnHold(byte sensorCol, byte sensorRow) {
  byte activeScaleId = Global.activeNotes;

  // TODO: Temporarily disabled. How should we access custom color grid?
  // if (lightSettings == LIGHTS_ACTIVE && sensorRow == 3) {
  //   cellTouched(ignoredCell);
  //   loadCustomLedLayer(getActiveCustomLedPattern());
  //   setDisplayMode(displayCustomLedsEditor);
  //   updateDisplay();
  // }
  if (sensorCol == 1 && sensorRow == LIGHTS_ACCENT && lightSettings == LIGHTS_ACCENT) {
    // The [GLOBAL SETTINGS]->[ACCENT] button was long-held.
    // Permute the colors into fifths.
    for (byte note = 1; note <= 5; note+=2) {
      byte curColor = Global.scaleNoteColors[note];
      byte fifthColor = Global.scaleNoteColors[(note+6)%12];
      Global.scaleNoteColors[(note+6)%12] = curColor;
      Global.scaleNoteColors[note] = fifthColor;
    }
    updateDisplay();
  }

  byte pressedNote = noteAtCell(sensorCol, sensorRow);
  if (pressedNote>11) {
    return;
  }
  if (lightSettings == LIGHTS_ACCENT) {
      // In [GLOBAL SETTINGS]->[ACCENT], a scale note was long-held.
      // Mark this color as the color offset, which means rotate the colors to start from this color.
      if (scaleGetAssignedColorOffset() != pressedNote) {
        // TODO: What if user sets a non-color as the root color? How to pulse?
        scaleSetAssignedColorOffset(pressedNote);
      } else {
        scaleSetAssignedColorOffset(-1);
      }
      updateDisplay();
  }
  if (lightSettings == LIGHTS_MAIN) {
      // In [GLOBAL SETTINGS]->[MAIN], a note on the scale was long-held.
      // Use this to set the mode of the scale, i.e., mark the start of the scale pattern.

      // TODO: Fix edge cases
      // if (scaleContainsNote(activeScaleId, pressedNote) &&
      //     scaleGetAssignedMode(activeScaleId) != pressedNote) {
      //   scaleSetAssignedMode(activeScaleId, pressedNote);
      // } else {
      //   scaleSetAssignedMode(activeScaleId, -1);
      // }

      if (scaleContainsNote(activeScaleId, pressedNote) &&
          scaleGetAssignedMode(activeScaleId) != pressedNote) {
        scaleSetAssignedMode(activeScaleId, pressedNote);      
      } else {
        scaleSetAssignedMode(activeScaleId, -1);
      }
      updateDisplay();
  }
}
