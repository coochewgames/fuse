# Simplified FUSE

## Objective
To output memory data to a local port, allowing ML to poll memory values in learning to play a game.
Read in the dat files with the Z80 instruction sets at the initialisation of the executable, as opposed to being used to create the source code.

## Project Goals
 - Remove (Perl) build script code generation and replace with C
 - Remove small memory option from build
 - Remove gotos from code
 - Simplify use of macros to make code more readable

### Expectations from build updates
 - Can run on a Pi 0
 - Can run on an M-series Mac

## Building
Main development platform is the command line on an M-processor using OS X.  The build for SDL requires SDL 1.2.4 or higher which I could not get to successfully install in the development environment, so this is based on GTK.

### Steps for libspectrum
  -  `brew install glib`
  -  `./configure`
  -  `make`
  -  `sudo make install`

### Steps for building and running FUSE
 - `brew install gtk+3`
 - `brew install automake`
 - `./autogen.sh`
 - `./configure`
 - `make`
 - `./fuse`

## ML Bridge (Milestone 1)
Set environment variables before starting `fuse`:

- `FUSE_ML_MODE=1` enables command-driven ML mode.
- `FUSE_ML_SOCKET=/tmp/fuse-ml.sock` optionally sets the UNIX socket path.
- `FUSE_ML_RESET_SNAPSHOT=/path/to/state.szx` optionally sets reset target state.
- `FUSE_ML_VISUAL=1` enables visual rendering in ML mode (default is headless).
- `FUSE_ML_VISUAL_PACE_MS=16` optionally paces each stepped frame in visual mode.
- `FUSE_ML_GAME=MANIC_MINER` enables the Stage 2.3 game adapter.
- `FUSE_ML_ACTION_KEYS=0,54,55,48` optionally overrides action->key mapping.
- `FUSE_ML_REWARD_ADDR=0x0000` optionally tracks reward as byte delta at address.
- `FUSE_ML_DONE_ADDR=0x0000` optionally tracks episode end address.
- `FUSE_ML_DONE_VALUE=0` optionally sets the done-match value (default `0`).

In ML mode, sound and gdbserver are disabled, and the emulator listens on the
socket for line-based commands:

- `PING`
- `RESET`
- `KEYDOWN <key>`
- `KEYUP <key>`
- `STEP <frames>`
- `READ <address> <length>`
- `GETINFO`
- `GETSCREEN`
- `MODE`
- `MODE HEADLESS`
- `MODE VISUAL [pace_ms]`
- `GAME`
- `ACT <action> <frames>`
- `QUIT`

Responses are text lines:

- `OK ...` for success
- `DATA <hex bytes>` for memory reads
- `INFO <frame_count> <tstates> <width> <height>` for emulator state
- `SCREEN <width> <height> IDX8_HEX <hex bytes>` for palette-index frame data
- `MODE <HEADLESS|VISUAL> <pace_ms>` for current run mode
- `GAME OFF` when no adapter is active
- `GAME ON <name> <actions> <reward_addr|-> <done_addr|-> <done_value>` for adapter settings
- `ACT <frame_count> <reward> <done>` after action+step execution
- `ERR ...` for failures

For `MANIC_MINER`, the default actions are:
- `0` no-op
- `1` key `6` (left)
- `2` key `7` (right)
- `3` key `0` (jump)

## Notes
With the refactoring, this version will require the dat files with the Z80 instruction sets to be available with the executable.  This will entail that they can be updated directly without the need to rebuild, which aligns with the ability to experiment with the different aspects of the emulator.

The Free Unix Spectrum Emulator (Fuse) 1.6.0
============================================

Fuse (the Free Unix Spectrum Emulator) was originally, and somewhat
unsurprisingly, an emulator of the ZX Spectrum (a popular 1980s home
computer, especially in the UK) for Unix. However, it has now also
been ported to Mac OS X, which may or may not count as a Unix variant
depending on your advocacy position and Windows which definitely isn't
a Unix variant. Fuse also emulates some of the better-known ZX Spectrum
clones as well.

What Fuse does have:

* Accurate Spectrum 16K/48K/128K/+2/+2A/+3 emulation.
* Working Spectrum +3e and SE, Timex TC2048, TC2068 and TS2068,
  Pentagon 128, "512" (Pentagon 128 with extra memory) and 1024 and
  Scorpion ZS 256 emulation.
* Runs at true Speccy speed on any computer you're likely to try it on.
* Support for loading from .tzx files, including accelerated loading.
* Sound (on systems supporting the Open Sound System, SDL, OpenBSD/
  Solaris's /dev/audio, CoreAudio or PulseAudio).
* Emulation of most of the common joysticks used on the Spectrum
  (including Kempston, Sinclair and Cursor joysticks).
* Emulation of some of the printers you could attach to a Spectrum.
* Support for the RZX input recording file format, including
  rollback and 'competition mode'.
* Emulation of the Currah µSource, Interface 1, Kempston mouse,
  Multiface One/128/3 and TTX2000S interfaces.
* Emulation of the Covox, Fuller audio box, Melodik and SpecDrum audio
  interfaces.
* Emulation of the DivIDE, DivMMC, Spectrum +3e, ZXATASP, ZXCF and ZXMMC
  storage interfaces.
* Emulation of the Beta 128, +D, Didaktik 80/40, DISCiPLE and Opus Discovery
  disk interfaces.
* Emulation of the Spectranet and SpeccyBoot network interfaces.
* Emulation of the TTX2000 S Teletext adapter.
* Support for the Recreated ZX Spectrum Bluetooth keyboard.

Help! <xyz> doesn't work
------------------------

If you're having a problem using/running/building Fuse, the two places
you're most likely to get help are the development mailing list
<fuse-emulator-devel@lists.sf.net> or the official forums at
<http://sourceforge.net/p/fuse-emulator/discussion/>.

What you'll need to run Fuse
----------------------------

Unix, Linux, BSD, etc.

Required:

* X, SDL or framebuffer support. If you have GTK, you'll get
  a (much) nicer user interface under X.
* libspectrum: this is available from
  http://fuse-emulator.sourceforge.net/libspectrum.php

Optional:

* Other libraries will give you some extended functionality:
  * libgcrypt: the ability to digitally sign input recordings (note that
    Fuse requires version 1.1.42 or later).
  * libpng: the ability to save screenshots
  * libxml2: the ability to load and save Fuse's current configuration
  * zlib: support for compressed RZX files

If you've used Fuse prior to version 0.5.0, note that the external
utilities (tzxlist, etc) are now available separately from Fuse
itself. See http://fuse-emulator.sourceforge.net/ for details.

Mac OS X

* Either the native port by Fredrick Meunier, or the original version
  will compile on OS X 10.3 (Panther) or later.
* On Mac OS X Lion you will need to use clang as gcc-llvm-4.2.1 fails to
  correctly compile process_z80_opcodes.c.

Windows

* The Win32 and SDL UIs can be used under Windows.
* pthreads-win32 library will give the ability to use posix threads, needed by
  some peripherals.

Building Fuse
-------------

See the file `INSTALL' for more detailed information.

Closing comments
----------------

Fuse has its own home page, which you can find at:

http://fuse-emulator.sourceforge.net/

and contains much of the information listed here. 

News of new versions of Fuse (and other important Fuse-related
announcements) are distributed via the fuse-emulator-announce mailing
list on SourceForge; see
http://lists.sourceforge.net/lists/listinfo/fuse-emulator-announce
for details on how to subscribe and the like.

If you've got any bug reports, suggestions or the like for Fuse, or
just want to get involved in the development, this is coordinated via
the fuse-emulator-devel mailing list,
http://lists.sourceforge.net/lists/listinfo/fuse-emulator-devel
and the Fuse project page on SourceForge,
http://sourceforge.net/projects/fuse-emulator/

For Spectrum discussions not directly related to Fuse, visit either the
Usenet newsgroup `comp.sys.sinclair' or the World of Spectrum forums
<http://www.worldofspectrum.org/forums/>.

Philip Kendall <philip-fuse@shadowmagic.org.uk>
27th February, 2021
