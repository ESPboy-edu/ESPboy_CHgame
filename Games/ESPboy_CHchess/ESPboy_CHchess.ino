// CHChess - isometric casino chess for the CHGame handheld (CH32X035,
// 128x128 ST7735, piezo). Rules and CPU: the ch2k engine from ArduChess by
// Peter Brown (tiberiusbrown), MPL-2.0 (src/engine/ch2k.hpp).
//
// Frame loop: logic runs while the previous frame is still going out over
// DMA; drawing waits for it (one framebuffer), then the new frame is sent.
// The same frame runs from inside the CPU's search (see src/Frame.h), so the
// game keeps moving while the engine thinks.

#include "config.h"
#include "src/gfx/ESPboyGfx.h"
#include "src/CHGame.h"
#include "src/Frame.h"
#include "ESPboyInit.h"

ESPboyInit myESPboy;

void setup() {
    Serial.begin(115200);
    myESPboy.begin("CHchess");

    Serial.println(ESP.getFreeHeap());
    
    gfx_init_espboy(&myESPboy.tft);

    arduboy.boot(&myESPboy);
    frame::begin();
    arduboy.setFrameRate(CHCH_FPS);
}

void loop() {
    frame::run(false);
}
