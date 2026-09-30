## About CHGame

**CHGame** is an open-source, ultra-low-cost retro handheld gaming platform and developer's kit (created by the team behind Arduboy). Built around an affordable RISC-V microcontroller, CHGame brings full-color retro gaming, custom graphics drivers, and sound capabilities to micro-hardware enthusiasts.

### CHGame Hardware Specifications
* **MCU:** CH32X035G8U6 (32-bit RISC-V @ 48 MHz)
* **Memory:** 64 KB Flash / 20 KB SRAM
* **Display:** 1.44" ST7735 Full-Color TFT LCD (128×128 pixels, 16-bit color)
* **Storage & Audio:** MicroSD slot, Piezo speaker
* **Inputs:** 8 tactile buttons + User LED

https://chgame.website

### Porting to ESPboy
While CHGame relies on a 48 MHz RISC-V chip with 20 KB RAM and a 128x128 display, the **ESPboy** (powered by an ESP8266 @ 80/160 MHz with 80 KB RAM and a 128x128 color LCD) offers more than enough horsepower and memory to run CHGame titles smoothly.

These ports adapt CHGame games and applications to run natively on ESPboy, adjusting button mappings, display drivers, and audio outputs to take full advantage of ESPboy's hardware.