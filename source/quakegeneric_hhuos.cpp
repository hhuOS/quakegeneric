/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <time.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <cstdio>
#include "quakegeneric.h"
#include "quakegeneric.h"
#include "lib/util/base/Address.h"
#include "lib/util/graphic/Ansi.h"
#include "lib/util/io/file/File.h"
#include "lib/util/graphic/LinearFrameBuffer.h"
#include "lib/util/io/key/KeyDecoder.h"
#include "lib/util/io/key/layout/DeLayout.h"
#include "lib/util/game/Engine.h"

unsigned char pal[768];
Util::Graphic::LinearFrameBuffer *lfb = nullptr;
Util::Io::KeyDecoder *kd = nullptr;

int main(int argc, char *argv[]) {
    if (!Util::Io::File::changeDirectory("/user/quake")) {
        Util::System::error << "quake: '/user/quake' not found!" << Util::Io::PrintStream::endl << Util::Io::PrintStream::flush;
        return -1;
    }

    Util::Graphic::Ansi::prepareGraphicalApplication(true);
    auto lfbFile = Util::Io::File("/device/lfb");
    lfb = new Util::Graphic::LinearFrameBuffer(lfbFile);
    lfb->clear();

    double oldtime, newtime;

    QG_Create(argc, argv);

    oldtime = (static_cast<double>(clock()) / CLOCKS_PER_SEC) - 0.1;
    while (true) {
        newtime = static_cast<double>(clock()) / CLOCKS_PER_SEC;
        QG_Tick(newtime - oldtime);
        oldtime = newtime;
    }
}

void QG_Init(void) {
    kd = new Util::Io::KeyDecoder(new Util::Io::DeLayout());
}

int QG_GetKey(int *down, int *key) {
    if (!stdin->isReadyToRead() || kd == nullptr) {
        return 0;
    } else {
        uint8_t scancode = fgetc(stdin);

        if ((scancode & ~0x80) == 0x1d) {
            *down = !(scancode & 0x80);
            *key = K_CTRL;
            return 1;
        }
        if ((scancode & ~0x80) == 0x38) {
            *down = !(scancode & 0x80);
            *key = K_ALT;
            return 1;
        }

        if (kd->parseScancode(scancode)) {
            auto k = kd->getCurrentKey();
            if (!k.isValid()) {
                return 0;
            }

            *down = k.isPressed() ? 1:0;

            if (k.getScancode() >= 0x3b && k.getScancode() <= 0x44) { // handle F1-10
                *key = k.getScancode() + 0x80;
                return 1;
            }

            switch(k.getScancode()) {
                case Util::Io::Key::UP:
                    *key = K_UPARROW;
                    return 1;
                case Util::Io::Key::DOWN:
                    *key = K_DOWNARROW;
                    return 1;
                case Util::Io::Key::LEFT:
                    *key = K_LEFTARROW;
                    return 1;
                case Util::Io::Key::RIGHT:
                    *key = K_RIGHTARROW;
                    return 1;
                case Util::Io::Key::SPACE:
                    *key = K_SPACE;
                    return 1;
                case Util::Io::Key::ESC:
                    *key = K_ESCAPE;
                    return 1;
                case Util::Io::Key::ENTER:
                    *key = K_ENTER;
                    return 1;
                case Util::Io::Key::TAB:
                    *key = K_TAB;
                    return 1;
                case Util::Io::Key::BACKSPACE:
                    *key = K_BACKSPACE;
                    return 1;
                default:
                    if (k.getAscii()) {
                        *key = tolower(k.getAscii());
                        return 1;
                    }

                    return 0;
            }
        }
    }

    return 0;
}

void QG_GetJoyAxes(float *axes) {
    *axes = 0;
}

void QG_GetMouseMove(int *x, int *y) {
    *x = 0;
    *y = 0;
}

void QG_Quit(void) {
    Util::Graphic::Ansi::cleanupGraphicalApplication();
    delete lfb;
    delete kd;
}

void QG_DrawFrame(void *pixels) {
    if (lfb == nullptr) {
        return;
    }

    auto offsetX = lfb->getResolutionX() - QUAKEGENERIC_RES_X * 2 > 0 ? (lfb->getResolutionX() - QUAKEGENERIC_RES_X * 2) / 2 : 0;
    auto offsetY = lfb->getResolutionY() - QUAKEGENERIC_RES_Y * 2 > 0 ? (lfb->getResolutionY() - QUAKEGENERIC_RES_Y * 2) / 2 : 0;

    auto screenBuffer = lfb->getBuffer().add(offsetX * 4 + offsetY * lfb->getPitch());

    for (uint32_t y = 0; y < QUAKEGENERIC_RES_Y * 2; y++) {
        for (uint32_t x = 0; x < QUAKEGENERIC_RES_X * 2; x += 2) {
            uint8_t pixel = ((uint8_t*) pixels)[(y / 2) * QUAKEGENERIC_RES_X + (x / 2)];
            uint8_t *entry = &((uint8_t*) pal)[pixel * 3];
            uint64_t value = (*(entry) << 16) + (*(entry + 1) << 8) + *(entry + 2);
            screenBuffer.add(x * 4).setLong((value << 32) | value);
        }

        screenBuffer = screenBuffer.add(lfb->getPitch());
    }
}

void QG_SetPalette(unsigned char palette[768]) {
    memcpy(pal, palette, 768);
}