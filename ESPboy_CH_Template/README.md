Here is the complete, detailed guide for porting CH-games (like CHChess) to the ESPboy platform using the **CHCore Hardware Abstraction Layer (HAL)** approach.

The guide is provided in both English and Russian.

---

# 🇬🇧 English: The Ultimate Guide to Porting CH-Games to ESPboy

This guide uses the **CHCore HAL** pattern. Instead of rewriting the game's logic, we create a transparent layer that mimics the original CH32V (RISC-V) hardware but safely translates commands for the ESP8266 (Xtensa).

### Phase 1: Project Structure & Core Files

1. **Create the Project Directory:** Create a folder named `ESPboy_CHGame`.
2. **Add ESPboy Libraries:** Create a `lib/` folder. Place the standard ESPboy files inside (`ESPboyInit.h/.cpp`, `ESPboyLED.h/.cpp`, `ESPboyMCP.h/.cpp`, and `nbSPI.h` for asynchronous DMA-like SPI transfers).


3. **Create the CHCore Folder:** Inside `src/`, create a folder named `CHCore/`. Place your universal HAL files here:
* `CHGame.h` & `CHGame.cpp`: Maps ESPboy buttons (`myESPboy.getKeys()`) to standard CHGame button masks (`A_BUTTON`, `UP_BUTTON`, etc.).


* `CHGfx.h` & `CHGfx.cpp`: Translates original 4bpp CHGfx drawing calls to the `TFT_eSPI` library, utilizing dynamic memory allocation (`new uint8_t[]`) and async `nbSPI` output.


* `RamFunc.h`: Contains an empty `#define RAMFUNC(name)` to strip out CH32V linker directives.


* `ESPboy_AudioCore.hpp`: The `Ticker`-based audio engine.
* `ESPboy_SaveCore.hpp`: The `EEPROM.h`-based save engine.



### Phase 2: Integrating the Game Source

1. **Copy the Source Code:** Copy the original game's `src/` folder (engine, logic, graphics) into your project.
2. **Delete Original Hardware Files:** Delete the game's original `CHGfx.h`, `CHGfx.cpp`, `CHGame.h`, and `CHGame.cpp` files.
3. **Mass Include Update:** Run a "Find and Replace in Files" across the entire project:
* **Find:** `#include <CHGfx.h>`
* **Replace with:** `#include "../CHCore/CHGfx.h"`



### Phase 3: Adapting Audio and Save Engines (Header-Only Trick)

The `Audio.cpp` and `Save.cpp` files contain both game-specific data and hardware-specific execution code.

1. **Audio.cpp:**
* Keep the top part containing `struct Step`, the `PROGMEM` melody arrays, and `SfxDef DEFS`.


* Delete everything below it (the `TIM1` and `GPIOB` hardware timers).


* At the very end of the file, add: `#include "../CHCore/ESPboy_AudioCore.hpp"`.


2. **Save.cpp:**
* Keep the `MAGIC`, `VERSION` constants, and the `struct Record` definition.


* Delete the CH32V `FLASH` controller manipulation code (e.g., `FLASH->CTLR`).


* At the very end of the file, add: `#include "../CHCore/ESPboy_SaveCore.hpp"`.



### Phase 4: Fixing ESP8266 Memory Crashes (Exception 3: LoadStoreError)

The ESP8266 crashes if you read unaligned data directly from Flash memory (`PROGMEM`).

1. **Add PROGMEM:** In `Assets.cpp`, ensure all large arrays (`PIECE_PAWN`, `PIECE_ART`, etc.) have the `PROGMEM` attribute.


2. **Safe Array Reads:** In files like `Draw.cpp` and `Stage.cpp`, wrap direct array accesses with `pgm_read_byte()`.
* *Example:* Change `remap[c]` to `pgm_read_byte(&remap[c])`.




3. **Safe Struct Reads:** If the game reads a struct from PROGMEM (like `PieceArt a = art(p)`), rewrite the getter function to copy the struct into RAM using `memcpy_P`.



### Phase 5: Final Configuration

1. **Check config.h:** Ensure `#define CHCH_LEAN 0` so that menus like OPTIONS are compiled into the game.


2. **Main .ino File:** Create `ESPboy_CHGame.ino` in the root folder, `#include "ESPboyInit.h"`, initialize it in `setup()`, and call the game's `update()` and `render()` loop inside Arduino's `loop()`. Compile and upload!

---

# 🇷🇺 Русский: Подробное руководство по портированию CH-Игр на ESPboy

В этом руководстве используется паттерн **CHCore HAL** (Слой аппаратных абстракций). Вместо того чтобы переписывать логику игры, мы создаем "прозрачный" слой, который имитирует оригинальное железо CH32V (RISC-V), но безопасно переводит его команды для ESP8266 (Xtensa).

### Этап 1: Структура проекта и файлы ядра

1. **Создайте папку проекта:** Назовите её `ESPboy_CHGame`.
2. **Добавьте библиотеки ESPboy:** Создайте папку `lib/`. Поместите туда стандартные файлы ESPboy (`ESPboyInit.h/.cpp`, `ESPboyLED.h/.cpp`, `ESPboyMCP.h/.cpp`, а также `nbSPI.h` для асинхронного вывода графики, эмулирующего DMA).


3. **Создайте папку CHCore:** Внутри `src/` создайте папку `CHCore/`. Разместите в ней ваши универсальные файлы ядра:
* `CHGame.h` и `CHGame.cpp`: Связывают кнопки ESPboy (`myESPboy.getKeys()`) со стандартными масками кнопок игры (`A_BUTTON`, `UP_BUTTON` и т.д.).


* `CHGfx.h` и `CHGfx.cpp`: Переводят оригинальные 4-битные вызовы отрисовки на библиотеку `TFT_eSPI`, используют динамическое выделение памяти (`new uint8_t[]`) и быстрый асинхронный вывод через `nbSPI`.


* `RamFunc.h`: Содержит пустой макрос `#define RAMFUNC(name)`, который обезвреживает инструкции линкера CH32V.


* `ESPboy_AudioCore.hpp`: Звуковой движок на базе прерываний `Ticker`.
* `ESPboy_SaveCore.hpp`: Движок сохранений на базе `EEPROM.h`.



### Этап 2: Интеграция исходного кода игры

1. **Копирование исходников:** Скопируйте папку `src/` из оригинальной игры в ваш проект.
2. **Удаление старого железа:** Удалите оригинальные файлы игры `CHGfx.h`, `CHGfx.cpp`, `CHGame.h` и `CHGame.cpp`.
3. **Массовая замена путей:** Выполните поиск с заменой по всем файлам проекта:
* **Найти:** `#include <CHGfx.h>`
* **Заменить на:** `#include "../CHCore/CHGfx.h"`



### Этап 3: Адаптация звука и сохранений (Трюк с шаблонами)

Файлы `Audio.cpp` и `Save.cpp` содержат как игровые данные, так и код прямого управления железом CH32V.

1. **Audio.cpp:**
* Оставьте верхнюю часть: `struct Step`, массивы мелодий с `PROGMEM` и `SfxDef DEFS`.


* Удалите всё, что ниже (аппаратные таймеры `TIM1` и `GPIOB`).


* В самом конце файла добавьте: `#include "../CHCore/ESPboy_AudioCore.hpp"`.


2. **Save.cpp:**
* Оставьте константы `MAGIC`, `VERSION` и структуру `struct Record`.


* Удалите код управления контроллером `FLASH` процессора CH32V.


* В самом конце файла добавьте: `#include "../CHCore/ESPboy_SaveCore.hpp"`.



### Этап 4: Исправление сбоев памяти ESP8266 (Exception 3: LoadStoreError)

ESP8266 уходит в краш, если вы читаете невыровненные данные напрямую из Flash-памяти (`PROGMEM`).

1. **Добавление PROGMEM:** В файле `Assets.cpp` убедитесь, что все большие массивы (спрайты, уровни) имеют атрибут `PROGMEM`.


2. **Безопасное чтение массивов:** В файлах `Draw.cpp`, `Stage.cpp` и `Mask.cpp` оберните прямое чтение из массивов макросом `pgm_read_byte()`.
* *Пример:* Измените `remap[c]` на `pgm_read_byte(&remap[c])`.




3. **Безопасное чтение структур:** Если игра читает структуру напрямую из `PROGMEM` (например, `PieceArt a = art(p)`), перепишите функцию-геттер так, чтобы она копировала структуру в RAM с помощью `memcpy_P`.



### Этап 5: Финальная сборка

1. **Проверка config.h:** Убедитесь, что `#define CHCH_LEAN 0`, чтобы меню настроек (OPTIONS) скомпилировалось.


2. **Главный файл .ino:** Создайте файл `ESPboy_CHGame.ino` в корне проекта. Подключите `"ESPboyInit.h"`, инициализируйте `myESPboy` внутри функции `setup()`, а в `loop()` вызывайте игровые функции `update()` и `render()`. Скомпилируйте и загрузите в ESPboy!