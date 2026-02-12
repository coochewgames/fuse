/* ml_bridge.c: machine-learning control bridge for Fuse
   Copyright (c) 2026

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include "event.h"
#include "fuse.h"
#include "input.h"
#include "machine.h"
#include "memory_pages.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "utils.h"

#include "z80/z80.h"

#include "ml_bridge.h"

static int fuse_ml_mode = 0;
static char *fuse_ml_socket_path = NULL;
static char *fuse_ml_reset_snapshot = NULL;
static int fuse_ml_server_fd = -1;

#ifndef WIN32

static int
fuse_ml_send( int fd, const char *data, size_t length )
{
  while( length ) {
    ssize_t written = write( fd, data, length );
    if( written < 0 ) {
      if( errno == EINTR ) continue;
      return 1;
    }
    data += written;
    length -= written;
  }

  return 0;
}

static int
fuse_ml_send_text( int fd, const char *text )
{
  return fuse_ml_send( fd, text, strlen( text ) );
}

static int
fuse_ml_read_line( int fd, char *buffer, size_t length )
{
  size_t pos = 0;

  if( !length ) return -1;

  while( 1 ) {
    char c;
    ssize_t read_result = read( fd, &c, 1 );

    if( read_result == 0 ) {
      if( pos == 0 ) return 0;
      break;
    } else if( read_result < 0 ) {
      if( errno == EINTR ) continue;
      return -1;
    }

    if( c == '\n' ) break;
    if( c == '\r' ) continue;

    if( pos + 1 >= length ) {
      do {
        read_result = read( fd, &c, 1 );
      } while( read_result == 1 && c != '\n' );
      return -2;
    }

    buffer[pos++] = c;
  }

  buffer[pos] = '\0';
  return 1;
}

static int
fuse_ml_parse_ulong( const char *text, unsigned long *value )
{
  char *endptr;
  unsigned long parsed;

  errno = 0;
  parsed = strtoul( text, &endptr, 0 );

  if( errno || endptr == text || *endptr ) return 1;

  *value = parsed;
  return 0;
}

static int
fuse_ml_key_event( input_event_type type, unsigned long key )
{
  input_event_t event;

  event.type = type;
  event.types.key.native_key = (input_key)key;
  event.types.key.spectrum_key = (input_key)key;

  return input_event( &event );
}

static int
fuse_ml_reset( void )
{
  if( fuse_ml_reset_snapshot && *fuse_ml_reset_snapshot )
    return snapshot_read( fuse_ml_reset_snapshot );

  return machine_reset( 1 );
}

static int
fuse_ml_step_frames( unsigned long frame_count )
{
  unsigned long i;

  for( i = 0; i < frame_count && !fuse_exiting; i++ ) {
    libspectrum_dword current_frame = spectrum_frame_count();
    size_t watchdog = 0;

    while( !fuse_exiting && spectrum_frame_count() == current_frame ) {
      z80_do_opcodes();
      event_do_events();

      if( ++watchdog > 20000000 ) return 1;
    }
  }

  return 0;
}

static int
fuse_ml_read_memory( int fd, unsigned long address, unsigned long length )
{
  static const char hex[] = "0123456789abcdef";
  char *response;
  size_t payload_length;
  unsigned long i;

  if( length > 0x10000 ) return fuse_ml_send_text( fd, "ERR length too large\n" );

  payload_length = 5 + (size_t)length * 2 + 1;
  response = libspectrum_new( char, payload_length + 1 );

  memcpy( response, "DATA ", 5 );

  for( i = 0; i < length; i++ ) {
    libspectrum_byte data = readbyte_internal( (libspectrum_word)( address + i ) );
    response[5 + i * 2] = hex[ data >> 4 ];
    response[5 + i * 2 + 1] = hex[ data & 0x0f ];
  }

  response[payload_length - 1] = '\n';
  response[payload_length] = '\0';

  if( fuse_ml_send( fd, response, payload_length ) ) {
    libspectrum_free( response );
    return 1;
  }

  libspectrum_free( response );

  return 0;
}

static int
fuse_ml_handle_command( int fd, char *line, int *disconnect )
{
  char *command = strtok( line, " \t" );
  char *arg1 = strtok( NULL, " \t" );
  char *arg2 = strtok( NULL, " \t" );
  char *extra = strtok( NULL, " \t" );

  if( !command ) return 0;

  if( !strcmp( command, "PING" ) ) {
    return fuse_ml_send_text( fd, "OK PONG\n" );
  } else if( !strcmp( command, "RESET" ) ) {
    if( arg1 || arg2 || extra ) return fuse_ml_send_text( fd, "ERR usage: RESET\n" );
    if( fuse_ml_reset() ) return fuse_ml_send_text( fd, "ERR reset failed\n" );
    return fuse_ml_send_text( fd, "OK\n" );
  } else if( !strcmp( command, "KEYDOWN" ) || !strcmp( command, "KEYUP" ) ) {
    unsigned long key;
    input_event_type type =
      !strcmp( command, "KEYDOWN" ) ? INPUT_EVENT_KEYPRESS : INPUT_EVENT_KEYRELEASE;

    if( !arg1 || arg2 || extra ) return fuse_ml_send_text( fd, "ERR usage: KEYDOWN|KEYUP <key>\n" );
    if( fuse_ml_parse_ulong( arg1, &key ) ) return fuse_ml_send_text( fd, "ERR invalid key\n" );
    if( fuse_ml_key_event( type, key ) ) return fuse_ml_send_text( fd, "ERR key event failed\n" );
    return fuse_ml_send_text( fd, "OK\n" );
  } else if( !strcmp( command, "STEP" ) ) {
    unsigned long frames;
    char response[80];

    if( !arg1 || arg2 || extra ) return fuse_ml_send_text( fd, "ERR usage: STEP <frames>\n" );
    if( fuse_ml_parse_ulong( arg1, &frames ) ) return fuse_ml_send_text( fd, "ERR invalid frame count\n" );
    if( fuse_ml_step_frames( frames ) ) return fuse_ml_send_text( fd, "ERR step failed\n" );

    snprintf( response, sizeof( response ), "OK %u\n",
              (unsigned int)spectrum_frame_count() );
    return fuse_ml_send_text( fd, response );
  } else if( !strcmp( command, "READ" ) ) {
    unsigned long address, length;

    if( !arg1 || !arg2 || extra ) return fuse_ml_send_text( fd, "ERR usage: READ <addr> <len>\n" );
    if( fuse_ml_parse_ulong( arg1, &address ) ||
        fuse_ml_parse_ulong( arg2, &length ) )
      return fuse_ml_send_text( fd, "ERR invalid address or length\n" );

    return fuse_ml_read_memory( fd, address, length );
  } else if( !strcmp( command, "QUIT" ) ) {
    fuse_exiting = 1;
    *disconnect = 1;
    return fuse_ml_send_text( fd, "OK\n" );
  }

  return fuse_ml_send_text( fd, "ERR unknown command\n" );
}

int
fuse_ml_init_socket( void )
{
  struct sockaddr_un addr;

  if( !fuse_ml_mode ) return 0;

  fuse_ml_server_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
  if( fuse_ml_server_fd < 0 ) {
    ui_error( UI_ERROR_ERROR, "ML bridge failed to create socket: %s",
              strerror( errno ) );
    return 1;
  }

  memset( &addr, 0, sizeof( addr ) );
  addr.sun_family = AF_UNIX;

  if( strlen( fuse_ml_socket_path ) >= sizeof( addr.sun_path ) ) {
    ui_error( UI_ERROR_ERROR, "ML bridge socket path too long: %s",
              fuse_ml_socket_path );
    close( fuse_ml_server_fd );
    fuse_ml_server_fd = -1;
    return 1;
  }

  strcpy( addr.sun_path, fuse_ml_socket_path );

  unlink( fuse_ml_socket_path );

  if( bind( fuse_ml_server_fd, (struct sockaddr*)&addr, sizeof( addr ) ) ) {
    ui_error( UI_ERROR_ERROR, "ML bridge bind failed for %s: %s",
              fuse_ml_socket_path, strerror( errno ) );
    close( fuse_ml_server_fd );
    fuse_ml_server_fd = -1;
    return 1;
  }

  if( listen( fuse_ml_server_fd, 1 ) ) {
    ui_error( UI_ERROR_ERROR, "ML bridge listen failed for %s: %s",
              fuse_ml_socket_path, strerror( errno ) );
    close( fuse_ml_server_fd );
    fuse_ml_server_fd = -1;
    unlink( fuse_ml_socket_path );
    return 1;
  }

  ui_error( UI_ERROR_INFO, "ML bridge listening on %s", fuse_ml_socket_path );

  return 0;
}

int
fuse_ml_loop( void )
{
  if( !fuse_ml_mode ) return 0;

  while( !fuse_exiting ) {
    int client_fd = accept( fuse_ml_server_fd, NULL, NULL );

    if( client_fd < 0 ) {
      if( errno == EINTR ) continue;
      ui_error( UI_ERROR_ERROR, "ML bridge accept failed: %s", strerror( errno ) );
      return 1;
    }

    if( fuse_ml_send_text( client_fd, "OK READY\n" ) ) {
      close( client_fd );
      continue;
    }

    while( !fuse_exiting ) {
      char line[1024];
      int read_status = fuse_ml_read_line( client_fd, line, sizeof( line ) );
      int disconnect = 0;

      if( read_status == 0 ) break;
      if( read_status == -2 ) {
        if( fuse_ml_send_text( client_fd, "ERR command too long\n" ) ) break;
        continue;
      }
      if( read_status < 0 ) break;
      if( !line[0] ) continue;

      if( fuse_ml_handle_command( client_fd, line, &disconnect ) ) break;
      if( disconnect ) break;
    }

    close( client_fd );
  }

  return 0;
}

void
fuse_ml_shutdown( void )
{
  if( fuse_ml_server_fd >= 0 ) {
    close( fuse_ml_server_fd );
    fuse_ml_server_fd = -1;
  }

  if( fuse_ml_socket_path ) {
    unlink( fuse_ml_socket_path );
    libspectrum_free( fuse_ml_socket_path );
    fuse_ml_socket_path = NULL;
  }

  if( fuse_ml_reset_snapshot ) {
    libspectrum_free( fuse_ml_reset_snapshot );
    fuse_ml_reset_snapshot = NULL;
  }
}

#else

int
fuse_ml_init_socket( void )
{
  if( fuse_ml_mode ) {
    ui_error( UI_ERROR_ERROR, "ML bridge mode is unsupported on this platform" );
    return 1;
  }

  return 0;
}

int
fuse_ml_loop( void )
{
  return 0;
}

void
fuse_ml_shutdown( void )
{
}

#endif

void
fuse_ml_configure_from_env( void )
{
  const char *mode = getenv( "FUSE_ML_MODE" );
  const char *socket_path = getenv( "FUSE_ML_SOCKET" );
  const char *reset_snapshot = getenv( "FUSE_ML_RESET_SNAPSHOT" );

  if( !mode || !*mode || !strcmp( mode, "0" ) ) return;

  fuse_ml_mode = 1;

  if( socket_path && *socket_path ) {
    fuse_ml_socket_path = utils_safe_strdup( socket_path );
  } else {
    fuse_ml_socket_path = utils_safe_strdup( "/tmp/fuse-ml.sock" );
  }

  if( reset_snapshot && *reset_snapshot )
    fuse_ml_reset_snapshot = utils_safe_strdup( reset_snapshot );

  settings_current.sound = 0;
  settings_current.sound_load = 0;
  settings_current.gdbserver_enable = 0;
}

int
fuse_ml_mode_enabled( void )
{
  return fuse_ml_mode;
}
