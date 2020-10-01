#pragma once

#include <raylib.h>
#include "controller.h"

#define mWidth 1280
#define mHeight 740
#define mVersion "1.0a"
#define MIN_NOTE_IDX 21
#define MAX_NOTE_IDX 108
#define NOTE_RANGE 88
#define ITEM_HEIGHT 20
#define ITEM_WIDTH 110
#define MENU_MARGIN 10
#define MAIN_MENU_HEIGHT 19
#define COLOR_WIDTH 200
#define COLOR_HEIGHT 200

enum colorSelections {
  SELECT_BG,
  SELECT_LINE,
  SELECT_NOTE,
  SELECT_SHEET,
  SELECT_NONE
};

enum displayModes {
  DISPLAY_LINE,
  DISPLAY_BAR,
  DISPLAY_BALL,
  DISPLAY_BALLLINE
};

extern controller ctr;
extern Font font;