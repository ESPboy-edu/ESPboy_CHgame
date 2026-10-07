// CHBackgammon - casino backgammon for the CHGame handheld (CH32X035,
// 128x128 ST7735, piezo), in the look of CHBlackjack and CHChess.
//
// The rules are in src/rules, the CPU (a small neural network that learned
// the game by playing itself, see tools/train) in src/ai, the game's flow in
// src/game, and everything you see and hear in src/table, src/stage and
// src/states.

#include "config.h"
#include "src/CHCore/CHGfx.h"
#include "src/CHCore/CHGame.h"
#include "src/CHCore/ESPboyInit.h"
#include "src/Frame.h"

//#include "src/CHCore/ESPboyTerminalGUI.h"
//#include "src/CHCore/ESPboyOTA2.h"

ESPboyInit myESPboy;

void setup() {
    myESPboy.begin("CHbackgammon");

      //Check OTA2
/*
  if (myESPboy.getKeys()&PAD_ACT || myESPboy.getKeys()&PAD_ESC) { 
     ESPboyTerminalGUI *terminalGUIobj = new ESPboyTerminalGUI(&myESPboy.tft, &myESPboy.mcp);
     ESPboyOTA2 *OTA2obj = new ESPboyOTA2(terminalGUIobj);
  }
  */  
    CHobj.boot(); 
    gfx_init(); 
    frame::begin();
    CHobj.setFrameRate(CHBG_FPS);
}

void loop() {
    frame::run();
}
