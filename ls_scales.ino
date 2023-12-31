
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

void scaleDisplayNoteLights(int scaleId) {
  int scaleBits = Global.mainNotes[scaleId];
  byte tonic = scaleGetTonic(scaleId);

  for (byte row = 0; row < 4; ++row) {
    for (byte col = 0; col < 3; ++col) {
      byte light = col + (row * 3);
      if (scaleBits & 1 << light) {
        if (light == tonic) {
          setLed(2+col, row, globalAltColor, cellOn);
        } else {
          setLed(2+col, row, globalColor, cellOn);
        }
      }
    }
  }
}
