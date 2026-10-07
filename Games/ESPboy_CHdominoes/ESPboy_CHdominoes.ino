// CHDominoes - casino dominoes for the CHGame handheld (CH32X035,
// 128x128 ST7735, piezo), in the look of CHBlackjack and CHChess: ALL FIVES
// and DRAW with the double-six set, against the CPU or between two players.
//
// The rules are in src/rules, the CPU in src/ai, the match's flow in
// src/game, where the tiles lie in src/table, and everything you see and
// hear in src/table, src/stage and src/states.

#include "config.h"
#include "src/CHCore/CHGfx.h"
#include "src/CHCore/CHGame.h"
#include "src/Frame.h"

#include "src/CHCore/ESPboyInit.h"

//#include "src/CHCore/ESPboyTerminalGUI.h"
//#include "src/CHCore/ESPboyOTA2.h"

ESPboyInit myESPboy;


void setup() {
    myESPboy.begin("CHdominoes");
    
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
    CHobj.setFrameRate(CHDM_FPS);
}

void loop() {
    frame::run();
}
