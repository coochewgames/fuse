/* uijoystick.h: Joystick emulation support
   Copyright (c) 2001-2003 Russell Marks, Philip Kendall, Darren Salt,
			   Fredrick Meunier

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

/* ui_joystick_* are called from joystick_*.
 * Do not use these directly; use the joystick_* wrappers.
 * UI-specific implementations are in ui/<ui>/<ui>joystick.c.
 * (If the UI cannot provide its own, it must use the default implementation;
 *  see uijoystick.c for details.)
 */

#ifndef FUSE_UI_UIJOYSTICK_H
#define FUSE_UI_UIJOYSTICK_H

#ifdef WORDS_BIGENDIAN

typedef struct
{
  unsigned unused : 3;
  unsigned fire   : 1;
  unsigned up     : 1;
  unsigned down   : 1;
  unsigned left   : 1;
  unsigned right  : 1;
} kempston_bits;

#else				/* #ifdef WORDS_BIGENDIAN */

typedef struct
{
  unsigned right  : 1;
  unsigned left   : 1;
  unsigned down   : 1;
  unsigned up     : 1;
  unsigned fire   : 1;
  unsigned unused : 3;
} kempston_bits;

#endif				/* #ifdef WORDS_BIGENDIAN */

typedef union
{
  libspectrum_byte byte;
  kempston_bits bits;
} kempston_type; 

int ui_joystick_init( void ); /* returns no. of joysticks initialised */
void ui_joystick_end( void );

/* Read function (returns data in Kempston format) */
libspectrum_byte ui_joystick_read( libspectrum_word port,
				   libspectrum_byte which );

#endif			/* #ifndef FUSE_UI_UIJOYSTICK_H */