/*
    z80_ops.c: Process the next opcode

    Copyright (c) 1999-2005 Philip Kendall, Witold Filipczyk
    Copyright (c) 2015 Stuart Brady
    Copyright (c) 2015 Gergely Szasz
    Copyright (c) 2015 Sergio Baldoví

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    Author contact information: philip-fuse@shadowmagic.org.uk
    Simplication refactoring by Roddy McNeill: rd.mcn@ccgapps.net.au
*/
#include <config.h>
#include <stdio.h>
#include "debugger/debugger.h"
#include "event.h"
#include "machine.h"
#include "memory_pages.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "peripherals/disk/didaktik.h"
#include "peripherals/disk/disciple.h"
#include "peripherals/disk/opus.h"
#include "peripherals/disk/plusd.h"
#include "peripherals/ide/divide.h"
#include "peripherals/ide/divmmc.h"
#include "peripherals/if1.h"
#include "peripherals/multiface.h"
#include "peripherals/spectranet.h"
#include "peripherals/ula.h"
#include "peripherals/usource.h"
#include "profile.h"
#include "rzx.h"
#include "settings.h"
#include "slt.h"
#include "svg.h"
#include "tape.h"
#include "z80.h"
#include "z80_macros.h"


/* Execute Z80 opcodes until the next event */
void z80_do_opcodes(void) {
    libspectrum_byte opcode = 0x00;
    libspectrum_byte last_Q;
    int even_m1 = machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_EVEN_M1;

    while (tstates < event_next_event) {
        /* Profiler */
        if (profile_active) {
            profile_map(PC);
        }

        /* RZX Playback End-of-Frame */
        if (rzx_playback) {
            if (R + rzx_instructions_offset >= rzx_instruction_count) {
                event_add(tstates, spectrum_frame_event);
                break;
            }
        }

        /* Debugger */
        if (debugger_mode != DEBUGGER_MODE_INACTIVE) {
            if (debugger_check(DEBUGGER_BREAKPOINT_TYPE_EXECUTE, PC)) {
                debugger_trap();
            }
        }

        /* Beta Disk */
        if (beta_available) {
            if (beta_active) {
                if ((!(machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY) ||
                     machine_current->ram.current_rom) &&
                    PC >= 16384) {
                    beta_unpage();
                }
            } else if ((PC & beta_pc_mask) == beta_pc_value &&
                       (!(machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY) ||
                        machine_current->ram.current_rom)) {
                beta_page();
            }
        }

        /* Other checks (e.g., plusd, disciple, etc.) */
        if (plusd_available && (PC == 0x0008 || PC == 0x003a || PC == 0x0066 || PC == 0x028e)) {
            plusd_page();
        }

        if (didaktik80_available) {
            if (PC == 0x0000 || PC == 0x0008) {
                didaktik80_page();
            } else if (PC == 0x1700) {
                didaktik80_unpage();
            }
        }

        if (disciple_available && (PC == 0x0001 || PC == 0x0008 || PC == 0x0066 || PC == 0x028e)) {
            disciple_page();
        }

        if (usource_available && PC == 0x2bae) {
            usource_toggle();
        }

        if (multiface_activated && PC == 0x0066) {
            multiface_setic8();
        }

        if (if1_available && (PC == 0x0008 || PC == 0x1708)) {
            if1_page();
        }

        if (settings_current.divide_enabled && (PC & 0xff00) == 0x3d00) {
            divide_set_automap(1);
        }

        if (settings_current.divmmc_enabled && (PC & 0xff00) == 0x3d00) {
            divmmc_set_automap(1);
        }

        if (spectranet_available && !settings_current.spectranet_disable) {
            if (PC == 0x0008 || ((PC & 0xfff8) == 0x3ff8)) {
                spectranet_page(0);
            }
            if (PC == spectranet_programmable_trap && spectranet_programmable_trap_active) {
                event_add(0, z80_nmi_event);
            }
        }

        /* Opcode fetch and execute */
        contend_read(PC, 4);

        if (even_m1 && (tstates & 1)) {
            if (++tstates == event_next_event) {
                break;
            }
        }

        opcode = readbyte_internal(PC);

        if (if1_available && PC == 0x0700) {
            if1_unpage();
        }

        if (settings_current.divide_enabled) {
            if ((PC & 0xfff8) == 0x1ff8) {
                divide_set_automap(0);
            } else if (PC == 0x0000 || PC == 0x0008 || PC == 0x0038 || PC == 0x0066 ||
                       PC == 0x04c6 || PC == 0x0562) {
                divide_set_automap(1);
            }
        }

        if (settings_current.divmmc_enabled) {
            if ((PC & 0xfff8) == 0x1ff8) {
                divmmc_set_automap(0);
            } else if (PC == 0x0000 || PC == 0x0008 || PC == 0x0038 || PC == 0x0066 ||
                       PC == 0x04c6 || PC == 0x0562) {
                divmmc_set_automap(1);
            }
        }

        PC++;
        R++;
        last_Q = Q;
        Q = 0;

        switch (opcode) {
#include "z80/opcodes_base.c"
        }
    }
}
