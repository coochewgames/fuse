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
#include "ml_game_adapter.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "utils.h"

#include "z80/z80.h"

#include "ml_bridge.h"

static int fuse_ml_mode = 0;
static int fuse_ml_visual_mode = 0;
static int fuse_ml_visual_pace_ms = 0;
static char *fuse_ml_socket_path = NULL;
static char *fuse_ml_reset_snapshot = NULL;
static int fuse_ml_server_fd = -1;

#ifndef WIN32

static int fuse_ml_apply_action( unsigned long action, unsigned long frames,
                                 long *reward, int *done,
                                 const char **error_text );

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
  int error;

  if( fuse_ml_reset_snapshot && *fuse_ml_reset_snapshot )
    error = snapshot_read( fuse_ml_reset_snapshot );
  else
    error = machine_reset( 1 );

  if( !error ) fuse_ml_game_resync();

  return error;
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

    if( fuse_ml_visual_mode && fuse_ml_visual_pace_ms > 0 )
      timer_sleep( fuse_ml_visual_pace_ms );
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

static void
fuse_ml_get_frame_dimensions( int *width, int *height )
{
  if( machine_current && machine_current->timex ) {
    *width = DISPLAY_SCREEN_WIDTH;
    *height = 2 * DISPLAY_SCREEN_HEIGHT;
  } else {
    *width = DISPLAY_ASPECT_WIDTH;
    *height = DISPLAY_SCREEN_HEIGHT;
  }
}

static int
fuse_ml_send_info( int fd )
{
  char response[96];
  int width, height;

  fuse_ml_get_frame_dimensions( &width, &height );

  snprintf( response, sizeof( response ), "INFO %u %u %d %d\n",
            (unsigned int)spectrum_frame_count(), (unsigned int)tstates,
            width, height );

  return fuse_ml_send_text( fd, response );
}

static int
fuse_ml_send_mode( int fd, const char *prefix )
{
  char response[80];

  snprintf( response, sizeof( response ), "%s %s %d\n", prefix,
            fuse_ml_visual_mode ? "VISUAL" : "HEADLESS",
            fuse_ml_visual_pace_ms );

  return fuse_ml_send_text( fd, response );
}

static int
fuse_ml_send_screen( int fd )
{
  static const char hex[] = "0123456789abcdef";
  char header[80];
  char chunk[4096];
  int width, height;
  int x, y;
  size_t used = 0;

  fuse_ml_get_frame_dimensions( &width, &height );

  snprintf( header, sizeof( header ), "SCREEN %d %d IDX8_HEX ",
            width, height );
  if( fuse_ml_send_text( fd, header ) ) return 1;

  for( y = 0; y < height; y++ ) {
    for( x = 0; x < width; x++ ) {
      int pixel = display_getpixel( x, y ) & 0xff;

      chunk[used++] = hex[ pixel >> 4 ];
      chunk[used++] = hex[ pixel & 0x0f ];

      if( used >= sizeof( chunk ) - 2 ) {
        if( fuse_ml_send( fd, chunk, used ) ) return 1;
        used = 0;
      }
    }
  }

  if( used && fuse_ml_send( fd, chunk, used ) ) return 1;

  return fuse_ml_send_text( fd, "\n" );
}

static int
fuse_ml_action_step( int fd, unsigned long action, unsigned long frames )
{
  long reward = 0;
  int done = 0;
  const char *error_text = NULL;
  char response[96];

  if( fuse_ml_apply_action( action, frames, &reward, &done, &error_text ) )
    return fuse_ml_send_text( fd, error_text );

  snprintf( response, sizeof( response ), "ACT %u %ld %d\n",
            (unsigned int)spectrum_frame_count(), reward, done );
  return fuse_ml_send_text( fd, response );
}

static int
fuse_ml_parse_bool( const char *text, int *value )
{
  unsigned long parsed = 0;

  if( !text || !value ) return 1;
  if( fuse_ml_parse_ulong( text, &parsed ) ) return 1;
  if( parsed > 1 ) return 1;

  *value = parsed ? 1 : 0;
  return 0;
}

static int
fuse_ml_apply_action( unsigned long action, unsigned long frames,
                      long *reward, int *done, const char **error_text )
{
  unsigned long action_keys[ FUSE_ML_GAME_MAX_KEYS_PER_ACTION ];
  size_t action_key_count = 0;
  unsigned long pressed_keys[ FUSE_ML_GAME_MAX_KEYS_PER_ACTION ];
  size_t pressed_count = 0;
  size_t i;
  int release_failed = 0;

  if( reward ) *reward = 0;
  if( done ) *done = 0;
  if( error_text ) *error_text = "ERR action failed\n";

  if( !fuse_ml_game_enabled() ) {
    if( error_text ) *error_text = "ERR game adapter disabled\n";
    return 1;
  }

  if( fuse_ml_game_get_action_keys( action, action_keys,
                                    ARRAY_SIZE( action_keys ),
                                    &action_key_count ) ) {
    if( error_text ) *error_text = "ERR invalid action\n";
    return 1;
  }

  for( i = 0; i < action_key_count; i++ ) {
    unsigned long key = action_keys[i];

    if( !key ) continue;

    if( fuse_ml_key_event( INPUT_EVENT_KEYPRESS, key ) ) {
      size_t j;

      for( j = pressed_count; j > 0; j-- )
        fuse_ml_key_event( INPUT_EVENT_KEYRELEASE, pressed_keys[j - 1] );

      if( error_text ) *error_text = "ERR key event failed\n";
      return 1;
    }

    pressed_keys[pressed_count++] = key;
  }

  if( fuse_ml_step_frames( frames ) ) {
    for( i = pressed_count; i > 0; i-- )
      fuse_ml_key_event( INPUT_EVENT_KEYRELEASE, pressed_keys[i - 1] );
    if( error_text ) *error_text = "ERR step failed\n";
    return 1;
  }

  for( i = pressed_count; i > 0; i-- ) {
    if( fuse_ml_key_event( INPUT_EVENT_KEYRELEASE, pressed_keys[i - 1] ) )
      release_failed = 1;
  }

  if( release_failed ) {
    if( error_text ) *error_text = "ERR key release failed\n";
    return 1;
  }

  if( fuse_ml_game_evaluate( reward, done ) ) {
    if( error_text ) *error_text = "ERR game evaluate failed\n";
    return 1;
  }

  return 0;
}

static int
fuse_ml_episode_step( int fd, unsigned long action, unsigned long frames,
                      int auto_reset )
{
  long reward = 0;
  int done = 0;
  int reset_performed = 0;
  int width, height;
  const char *error_text = NULL;
  char response[160];

  if( fuse_ml_apply_action( action, frames, &reward, &done, &error_text ) )
    return fuse_ml_send_text( fd, error_text );

  if( done && auto_reset ) {
    if( fuse_ml_reset() ) return fuse_ml_send_text( fd, "ERR reset failed\n" );
    reset_performed = 1;
  }

  fuse_ml_get_frame_dimensions( &width, &height );

  snprintf( response, sizeof( response ), "EPISODE %u %u %d %d %ld %d %d\n",
            (unsigned int)spectrum_frame_count(), (unsigned int)tstates,
            width, height, reward, done, reset_performed );
  return fuse_ml_send_text( fd, response );
}

static int
fuse_ml_handle_command( int fd, char *line, int *disconnect )
{
  char *command = strtok( line, " \t" );
  char *arg1 = strtok( NULL, " \t" );
  char *arg2 = strtok( NULL, " \t" );
  char *arg3 = strtok( NULL, " \t" );
  char *extra = strtok( NULL, " \t" );

  if( !command ) return 0;

  if( !strcmp( command, "PING" ) ) {
    return fuse_ml_send_text( fd, "OK PONG\n" );
  } else if( !strcmp( command, "RESET" ) ) {
    if( arg1 || arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: RESET\n" );
    if( fuse_ml_reset() ) return fuse_ml_send_text( fd, "ERR reset failed\n" );
    return fuse_ml_send_text( fd, "OK\n" );
  } else if( !strcmp( command, "KEYDOWN" ) || !strcmp( command, "KEYUP" ) ) {
    unsigned long key;
    input_event_type type =
      !strcmp( command, "KEYDOWN" ) ? INPUT_EVENT_KEYPRESS : INPUT_EVENT_KEYRELEASE;

    if( !arg1 || arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: KEYDOWN|KEYUP <key>\n" );
    if( fuse_ml_parse_ulong( arg1, &key ) ) return fuse_ml_send_text( fd, "ERR invalid key\n" );
    if( fuse_ml_key_event( type, key ) ) return fuse_ml_send_text( fd, "ERR key event failed\n" );
    return fuse_ml_send_text( fd, "OK\n" );
  } else if( !strcmp( command, "STEP" ) ) {
    unsigned long frames;
    char response[80];

    if( !arg1 || arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: STEP <frames>\n" );
    if( fuse_ml_parse_ulong( arg1, &frames ) ) return fuse_ml_send_text( fd, "ERR invalid frame count\n" );
    if( fuse_ml_step_frames( frames ) ) return fuse_ml_send_text( fd, "ERR step failed\n" );

    snprintf( response, sizeof( response ), "OK %u\n",
              (unsigned int)spectrum_frame_count() );
    return fuse_ml_send_text( fd, response );
  } else if( !strcmp( command, "READ" ) ) {
    unsigned long address, length;

    if( !arg1 || !arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: READ <addr> <len>\n" );
    if( fuse_ml_parse_ulong( arg1, &address ) ||
        fuse_ml_parse_ulong( arg2, &length ) )
      return fuse_ml_send_text( fd, "ERR invalid address or length\n" );

    return fuse_ml_read_memory( fd, address, length );
  } else if( !strcmp( command, "GETINFO" ) ) {
    if( arg1 || arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: GETINFO\n" );
    return fuse_ml_send_info( fd );
  } else if( !strcmp( command, "GETSCREEN" ) ) {
    if( arg1 || arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: GETSCREEN\n" );
    return fuse_ml_send_screen( fd );
  } else if( !strcmp( command, "MODE" ) ) {
    if( !arg1 ) return fuse_ml_send_mode( fd, "MODE" );

    if( !strcmp( arg1, "HEADLESS" ) ) {
      if( arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: MODE HEADLESS\n" );
      fuse_ml_visual_mode = 0;
      fuse_ml_visual_pace_ms = 0;
      return fuse_ml_send_mode( fd, "OK MODE" );
    }

    if( !strcmp( arg1, "VISUAL" ) ) {
      unsigned long pace = 0;

      if( arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: MODE VISUAL [pace_ms]\n" );
      if( arg2 && fuse_ml_parse_ulong( arg2, &pace ) )
        return fuse_ml_send_text( fd, "ERR invalid pace\n" );
      if( pace > 10000 )
        return fuse_ml_send_text( fd, "ERR pace too large\n" );

      fuse_ml_visual_mode = 1;
      fuse_ml_visual_pace_ms = (int)pace;
      return fuse_ml_send_mode( fd, "OK MODE" );
    }

    return fuse_ml_send_text( fd, "ERR mode must be HEADLESS or VISUAL\n" );
  } else if( !strcmp( command, "GAME" ) ) {
    char response[128];

    if( arg1 || arg2 || arg3 || extra ) return fuse_ml_send_text( fd, "ERR usage: GAME\n" );
    if( fuse_ml_game_info( response, sizeof( response ) ) )
      return fuse_ml_send_text( fd, "ERR game info unavailable\n" );
    return fuse_ml_send_text( fd, response );
  } else if( !strcmp( command, "ACT" ) ) {
    unsigned long action, frames;

    if( !arg1 || !arg2 || arg3 || extra )
      return fuse_ml_send_text( fd, "ERR usage: ACT <action> <frames>\n" );
    if( fuse_ml_parse_ulong( arg1, &action ) ||
        fuse_ml_parse_ulong( arg2, &frames ) )
      return fuse_ml_send_text( fd, "ERR invalid action or frame count\n" );

    return fuse_ml_action_step( fd, action, frames );
  } else if( !strcmp( command, "EPISODE_STEP" ) ) {
    unsigned long action, frames;
    int auto_reset = 0;

    if( !arg1 || !arg2 || extra )
      return fuse_ml_send_text( fd, "ERR usage: EPISODE_STEP <action> <frames> [auto_reset_0_or_1]\n" );
    if( fuse_ml_parse_ulong( arg1, &action ) ||
        fuse_ml_parse_ulong( arg2, &frames ) )
      return fuse_ml_send_text( fd, "ERR invalid action or frame count\n" );
    if( arg3 && fuse_ml_parse_bool( arg3, &auto_reset ) )
      return fuse_ml_send_text( fd, "ERR invalid auto_reset value\n" );

    return fuse_ml_episode_step( fd, action, frames, auto_reset );
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

  fuse_ml_game_resync();

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

  fuse_ml_game_shutdown();
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
  fuse_ml_game_shutdown();
}

#endif

int
fuse_ml_configure_from_env( void )
{
  const char *mode = getenv( "FUSE_ML_MODE" );
  const char *visual = getenv( "FUSE_ML_VISUAL" );
  const char *visual_pace = getenv( "FUSE_ML_VISUAL_PACE_MS" );
  const char *socket_path = getenv( "FUSE_ML_SOCKET" );
  const char *reset_snapshot = getenv( "FUSE_ML_RESET_SNAPSHOT" );
  unsigned long parsed_pace = 0;

  if( !mode || !*mode || !strcmp( mode, "0" ) ) return 0;

  fuse_ml_mode = 1;

  if( visual && *visual && strcmp( visual, "0" ) )
    fuse_ml_visual_mode = 1;

  if( visual_pace && *visual_pace &&
      !fuse_ml_parse_ulong( visual_pace, &parsed_pace ) &&
      parsed_pace <= 10000 ) {
    fuse_ml_visual_pace_ms = (int)parsed_pace;
  }

  if( !fuse_ml_visual_mode )
    fuse_ml_visual_pace_ms = 0;

  if( socket_path && *socket_path ) {
    fuse_ml_socket_path = utils_safe_strdup( socket_path );
  } else {
    fuse_ml_socket_path = utils_safe_strdup( "/tmp/fuse-ml.sock" );
  }

  if( reset_snapshot && *reset_snapshot )
    fuse_ml_reset_snapshot = utils_safe_strdup( reset_snapshot );

  if( fuse_ml_game_configure_from_env() ) return 1;

  settings_current.sound = 0;
  settings_current.sound_load = 0;
  settings_current.gdbserver_enable = 0;

  return 0;
}

int
fuse_ml_mode_enabled( void )
{
  return fuse_ml_mode;
}

int
fuse_ml_visual_mode_enabled( void )
{
  return fuse_ml_visual_mode;
}
