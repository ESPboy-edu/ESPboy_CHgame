#pragma GCC optimize("Os", "no-ipa-sra")
#include <Arduino.h>
#include "CHCore/CHGfx.h"
#include "../config.h"
#include "Frame.h"
#include "CHCore/CHGame.h"
#include "gfx/Palette.h"
#include "states/Screens.h"
#include "debug/Debug.h"
#include "audio/Audio.h"

namespace frame {

void begin() {
    dbg::paintStack();
    pal::init();
    screens::begin();
}

// Logic runs while the previous frame is still going out over DMA; drawing
// waits for it (one framebuffer), then the new frame is sent. The CPU's
// thinking is a slice of each logic tick (src/ai/Ai.h), so there is only
// this one loop.
bool run() {
    dbg::poll();
    if (!CHobj.nextFrame()) return false;
    dbg::markUpdateStart();
    // Logic runs at a fixed 60 Hz. If a heavy frame made drawing fall
    // behind, catch up (up to three ticks) before drawing again.
    uint8_t ticks = 0;
    do {
        CHobj.pollButtons();
        pal::tick();
        audio::update();
        screens::update();
    } while (++ticks < 3 && CHobj.nextFrame());
    gfx_wait();
    pal::commit();
    dbg::markRenderStart();
    screens::render(CHobj.frameCount);
    dbg::markRenderEnd();
    gfx_flushAsync();
    return true;
}

}  // namespace frame
