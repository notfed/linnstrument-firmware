
byte noteAtCell(byte col, byte row) {
  if (!(col >= 2 && col <= 4 && row >= 0 && row <= 3)) {
    return -1;
  }
  return (col - 2) + (row * 3);
}

void scaleToggleNote(byte scaleId, byte note) {
  if (note > 11) {
    return;
  }
  int* scale = &Global.mainNotes[scaleId];
  *scale ^= 1 << note;
}

boolean scaleContainsNote(byte scaleId, byte note) {
  if (note > 11) {
    return false;
  }
  int* scale = &Global.mainNotes[scaleId];
  return *scale & 1 << note;
}

byte scaleGetTonic(byte scaleId) {
  return Global.scaleTonic[scaleId] - 1;
}

void scaleSetTonic(byte scaleId, byte tonic) {
  if (tonic >= 0 && tonic <= 11) {
    Global.scaleTonic[scaleId] = tonic + 1;
  } else {
    Global.scaleTonic[scaleId] = 0;
  }
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
void scaleRedrawScaleSel() {
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
void scaleRedrawAccent() {
  byte scaleId = Global.activeNotes;
  for (byte row = 0; row <= 3; ++row) {
    for (byte col = 2; col <= 4; ++col) {
      byte note = noteAtCell(col, row);
      byte color = Global.noteAssignedColors[note];
      setLed(col, row, color, cellOn);
    }
  }
}

// [GLOBAL SETTINGS]->MAIN :: Draw all notes (of the current scale)
void scaleRedrawMain() {
  byte scaleId = Global.activeNotes;
  byte tonic = scaleGetTonic(scaleId);

  for (byte row = 0; row <= 3; ++row) {
    for (byte col = 2; col <= 4; ++col) {
      byte note = noteAtCell(col, row);
      if (scaleContainsNote(scaleId, note)) {
        if (note == tonic) {
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
          accentColor = colorCycle(accentColor, false);
          setLed(1, 1, accentColor, cellOn);
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

void scaleCellOnTouchEnd(byte sensorCol, byte sensorRow) {
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
    if (!customLedPatternActive) {
      if (getNoteAssignedColor(pressedNote) != accentColor) {
        setNoteAssignedColor(pressedNote, accentColor);
      } else {
        setNoteAssignedColor(pressedNote, COLOR_BLACK);
      }
    }
  }
  else if (lightSettings == LIGHTS_MAIN &&
      ensureCellBeforeHoldWait(COLOR_BLACK, cellOn)) {
    if (!customLedPatternActive) {
      byte activeScaleId = Global.activeNotes;
      // Toggle this note in the scale.
      scaleToggleNote(activeScaleId, pressedNote);
      // Clear the tonic note if it's not in the scale
      byte activeScaleTonic = scaleGetTonic(activeScaleId);
      if (!scaleContainsNote(activeScaleId, activeScaleTonic)) {
        scaleSetTonic(activeScaleId, -1);
      }
    }
  }
}

void scaleCellOnHold(byte sensorCol, byte sensorRow) {
  byte pressedNote = noteAtCell(sensorCol, sensorRow);
  if (pressedNote>11) {
    return;
  }

  // TODO: Temporarily disabled. How should we access custom color grid?
  // if (lightSettings == LIGHTS_ACTIVE && sensorRow == 3) {
  //   cellTouched(ignoredCell);
  //   loadCustomLedLayer(getActiveCustomLedPattern());
  //   setDisplayMode(displayCustomLedsEditor);
  //   updateDisplay();
  // }

  if (lightSettings == LIGHTS_MAIN) {
      // In [GLOBAL SETTINGS]->[MAIN], a scale note was long-held.
      byte activeScaleId = Global.activeNotes;
      byte pressedNote = noteAtCell(sensorCol, sensorRow);
      // If this note is in the scale, make this note the scale's tonic note.
      if (scaleContainsNote(activeScaleId, pressedNote) &&
          scaleGetTonic(activeScaleId) != pressedNote) {
        scaleSetTonic(activeScaleId, pressedNote);
      } else {
        scaleSetTonic(activeScaleId, -1);
      }
      updateDisplay();
  }
}

boolean scaleCellIsHoldable(byte sensorCol, byte sensorRow) {
  byte activeScaleId = Global.activeNotes;
  byte note = noteAtCell(sensorCol, sensorRow);
  if (note > 11) {
    return false;
  }
  if (lightSettings == LIGHTS_MAIN) {
    return scaleContainsNote(activeScaleId, note);
  } else if (lightSettings == LIGHTS_ACCENT) {
    return true;
  } else if (lightSettings == LIGHTS_ACTIVE) {
    return true;
  }
}