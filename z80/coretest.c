/* coretest.c: Test program for Fuse's Z80 core
   Copyright (c) 2003 Philip Kendall

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

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spectrum.h"
#include "ui/ui.h"
#include "z80.h"
#include "z80_macros.h"

static const char *progname;		/* argv[0] */

static int init_dummies( void );

DWORD tstates;
DWORD event_next_event;

/* 64Kb of RAM */
static BYTE initial_memory[ 0x10000 ], memory[ 0x10000 ];

spectrum_memory_read_function readbyte, readbyte_internal;
spectrum_memory_write_function writebyte;

spectrum_memory_contention_function contend_memory;
spectrum_port_contention_function contend_port;

static BYTE trivial_readbyte( WORD address );
static void trivial_writebyte( WORD address, BYTE b );
static DWORD trivial_contend_memory( WORD address );
static DWORD trivial_contend_port( WORD port );

static int read_test_file( const char *filename, DWORD *end_tstates );

static void dump_z80_state( void );
static void dump_memory_state( void );

int
main( int argc, char **argv )
{
  size_t i;

  progname = argv[0];

  if( argc < 2 ) {
    fprintf( stderr, "Usage: %s <testfile>\n", progname );
    return 1;
  }

  if( init_dummies() ) return 1;

  /* Initialise the tables used by the Z80 core */
  z80_init();

  /* Set up our trivial machine */
  readbyte = readbyte_internal = trivial_readbyte;
  writebyte = trivial_writebyte;
  contend_memory = trivial_contend_memory;
  contend_port = trivial_contend_port;

  /* Get ourselves into a known state */
  z80_reset();
  for( i = 0; i < 0x10000; i += 4 ) {
    memory[ i     ] = 0xde; memory[ i + 1 ] = 0xad;
    memory[ i + 2 ] = 0xbe; memory[ i + 3 ] = 0xef;
  }

  if( read_test_file( argv[1], &event_next_event ) ) return 1;

  /* Grab a copy of the memory for comparision at the end */
  memcpy( initial_memory, memory, 0x10000 );

  z80_do_opcodes();

  /* And dump our final state */
  dump_z80_state();
  dump_memory_state();

  return 0;
}

static BYTE
trivial_readbyte( WORD address )
{
  printf( "%5d MR %04x %02x\n", tstates, address, memory[ address ] );
  return memory[ address ];
}

static void
trivial_writebyte( WORD address, BYTE b )
{
  printf( "%5d MW %04x %02x\n", tstates, address, b );
  memory[ address ] = b;
}

static DWORD
trivial_contend_memory( WORD address )
{
  printf( "%5d MC %04x\n", tstates, address );
  return 0;
}

static DWORD
trivial_contend_port( WORD port )
{
  printf( "%5d PC %04x\n", tstates, port );
  return 0;
}

BYTE
readport( WORD port )
{
  /* For now, just return 0xff. May need to make this more complicated later */
  printf( "%5d PR %04x %02x\n", tstates, port, 0xff );
  return 0xff;
}

void
writeport( WORD port, BYTE b )
{
  /* Don't need to do anything here */
  printf( "%5d PW %04x %02x\n", tstates, port, b );
}

static int
read_test_file( const char *filename, DWORD *end_tstates )
{
  FILE *f;

  unsigned af, bc, de, hl, af_, bc_, de_, hl_, ix, iy, sp, pc;
  unsigned i, r, iff1, iff2, im;
  unsigned start_tstates, end_tstates2;
  unsigned address;

  f = fopen( filename, "r" );
  if( !f ) {
    fprintf( stderr, "%s: couldn't open `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  /* FIXME: how should we read/write our data types? */
  if( fscanf( f, "%x %x %x %x %x %x %x %x %x %x %x %x", &af, &bc,
	      &de, &hl, &af_, &bc_, &de_, &hl_, &ix, &iy, &sp, &pc ) != 12 ) {
    fprintf( stderr, "%s: first registers line in `%s' corrupt\n", progname,
	     filename );
    fclose( f );
    return 1;
  }

  AF  = af;  BC  = bc;  DE  = de;  HL  = hl;
  AF_ = af_; BC_ = bc_; DE_ = de_; HL_ = hl_;
  IX  = ix;  IY  = iy;  SP  = sp;  PC  = pc;

  if( fscanf( f, "%x %x %u %u %u %d %d %d", &i, &r, &iff1, &iff2, &im,
	      &z80.halted, &start_tstates, &end_tstates2 ) != 8 ) {
    fprintf( stderr, "%s: second registers line in `%s' corrupt\n", progname,
	     filename );
    fclose( f );
    return 1;
  }

  I = i; R = R7 = r; IFF1 = iff1; IFF2 = iff2; IM = im;
  tstates = start_tstates; *end_tstates = end_tstates2;

  while( 1 ) {

    if( fscanf( f, "%x", &address ) != 1 ) {

      if( feof( f ) ) break;

      fprintf( stderr, "%s: no address found in `%s'\n", progname, filename );
      fclose( f );
      return 1;
    }

    while( 1 ) {

      unsigned byte;

      if( fscanf( f, "%x", &byte ) != 1 ) {

	if( feof( f ) ) break;

	fprintf( stderr, "%s: no data byte found in `%s'\n", progname,
		 filename );
	fclose( f );
	return 1;
      }
    
      if( byte < 0 || byte > 255 ) break;

      memory[ address++ ] = byte;

    }
  }

  if( fclose( f ) ) {
    fprintf( stderr, "%s: couldn't close `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  return 0;
}

static void
dump_z80_state( void )
{
  printf( "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
	  AF, BC, DE, HL, AF_, BC_, DE_, HL_, IX, IY, SP, PC );
  printf( "%02x %02x %d %d %d %d %d\n", I, ( R7 & 0x80 ) | ( R & 0x7f ),
	  IFF1, IFF2, IM, z80.halted, tstates );
}

static void
dump_memory_state( void )
{
  size_t i;

  for( i = 0; i < 0x10000; i++ ) {

    if( memory[ i ] == initial_memory[ i ] ) continue;

    printf( "%4x ", i );

    while( i < 0x10000 && memory[ i ] != initial_memory[ i ] )
      printf( "%2x ", memory[ i++ ] );

    printf( "-1\n" );
  }
}

/* Error 'handing': dump core as these should never be called */

int
fuse_abort( void )
{
  abort();
}

int
ui_error( ui_error_level severity, const char *format, ... )
{
  va_list ap;

  va_start( ap, format );
  vfprintf( stderr, format, ap );
  va_end( ap );

  abort();
}

/*
 * Stuff below here not interesting: dummy functions and variables to replace
 * things used by Fuse, but not by the core test code
 */

#include "debugger/debugger.h"
#include "machine.h"
#include "scld.h"
#include "settings.h"

BYTE *slt[256];
size_t slt_length[256];

int
tape_load_trap( void )
{
  /* Should never be called */
  abort();
}

int
tape_save_trap( void )
{
  /* Should never be called */
  abort();
}

scld scld_last_dec;

size_t rzx_instruction_count;
int rzx_playback;
int rzx_instructions_offset;

enum debugger_mode_t debugger_mode;

int
debugger_check( debugger_breakpoint_type type, WORD value )
{
  /* Should never be called */
  abort();
}

int
debugger_trap( void )
{
  /* Should never be called */
  abort();
}

int trdos_active;

int
event_add( DWORD event_time, int type )
{
  /* Should never be called */
  abort();
}

fuse_machine_info *machine_current;
static fuse_machine_info dummy_machine;

settings_info settings_current;

/* Initialise the dummy variables such that we're running on a clean a
   machine as possible */
static int
init_dummies( void )
{
  debugger_mode = DEBUGGER_MODE_INACTIVE;
  dummy_machine.ram.current_rom = 0;
  machine_current = &dummy_machine;
  rzx_playback = 0;
  scld_last_dec.name.intdisable = 0;
  settings_current.slt_traps = 0;

  return 0;
}
