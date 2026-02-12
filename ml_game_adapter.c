/* ml_game_adapter.c: game-specific ML adapter helpers for Fuse
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

#include "input.h"
#include "memory_pages.h"
#include "ui/ui.h"
#include "utils.h"

#include "ml_game_adapter.h"

#define FUSE_ML_GAME_MAX_ACTIONS 32

static int fuse_ml_game_active = 0;
static char *fuse_ml_game_name = NULL;

static unsigned long
  fuse_ml_action_keys[ FUSE_ML_GAME_MAX_ACTIONS ][ FUSE_ML_GAME_MAX_KEYS_PER_ACTION ];
static size_t fuse_ml_action_key_counts[ FUSE_ML_GAME_MAX_ACTIONS ];
static unsigned long fuse_ml_action_count = 0;

static int fuse_ml_reward_enabled = 0;
static libspectrum_word fuse_ml_reward_addr = 0;
static unsigned long fuse_ml_reward_last = 0;
static int fuse_ml_reward_last_valid = 0;

static int fuse_ml_done_enabled = 0;
static libspectrum_word fuse_ml_done_addr = 0;
static unsigned long fuse_ml_done_value = 0;

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
fuse_ml_game_is_manic_miner( const char *text )
{
  char normalised[32];
  size_t i, j = 0;

  if( !text || !*text ) return 0;

  for( i = 0; text[i] && j + 1 < sizeof( normalised ); i++ ) {
    char c = text[i];

    if( c >= 'a' && c <= 'z' ) c = c - 'a' + 'A';
    if( c == '_' || c == '-' || c == ' ' ) continue;

    if( c < 'A' || ( c > 'Z' && ( c < '0' || c > '9' ) ) ) return 0;

    normalised[j++] = c;
  }

  normalised[j] = '\0';
  return !strcmp( normalised, "MANICMINER" );
}

static int
fuse_ml_game_parse_action_keys( const char *text )
{
  const char *cursor = text;
  unsigned long count = 0;

  if( !text || !*text ) return 1;

  while( *cursor ) {
    size_t key_count = 0;

    while( *cursor == ' ' || *cursor == '\t' || *cursor == ',' ) cursor++;
    if( !*cursor ) break;
    if( count >= FUSE_ML_GAME_MAX_ACTIONS ) return 1;

    while( 1 ) {
      unsigned long value;
      char *endptr;

      while( *cursor == ' ' || *cursor == '\t' ) cursor++;

      errno = 0;
      value = strtoul( cursor, &endptr, 0 );
      if( errno || endptr == cursor ) return 1;
      if( key_count >= FUSE_ML_GAME_MAX_KEYS_PER_ACTION ) return 1;

      fuse_ml_action_keys[count][key_count++] = value;
      cursor = endptr;

      while( *cursor == ' ' || *cursor == '\t' ) cursor++;
      if( *cursor != '+' ) break;
      cursor++;
    }

    if( !key_count ) return 1;
    fuse_ml_action_key_counts[count] = key_count;
    count++;

    while( *cursor == ' ' || *cursor == '\t' ) cursor++;
    if( !*cursor ) break;
    if( *cursor != ',' ) return 1;
    cursor++;
  }

  if( !count ) return 1;

  fuse_ml_action_count = count;
  return 0;
}

static void
fuse_ml_game_set_default_actions( void )
{
  fuse_ml_action_keys[0][0] = INPUT_KEY_NONE;
  fuse_ml_action_key_counts[0] = 1;

  fuse_ml_action_keys[1][0] = INPUT_KEY_q;
  fuse_ml_action_key_counts[1] = 1;

  fuse_ml_action_keys[2][0] = INPUT_KEY_w;
  fuse_ml_action_key_counts[2] = 1;

  fuse_ml_action_keys[3][0] = INPUT_KEY_space;
  fuse_ml_action_key_counts[3] = 1;

  fuse_ml_action_keys[4][0] = INPUT_KEY_q;
  fuse_ml_action_keys[4][1] = INPUT_KEY_space;
  fuse_ml_action_key_counts[4] = 2;

  fuse_ml_action_keys[5][0] = INPUT_KEY_w;
  fuse_ml_action_keys[5][1] = INPUT_KEY_space;
  fuse_ml_action_key_counts[5] = 2;

  fuse_ml_action_count = 6;
}

int
fuse_ml_game_configure_from_env( void )
{
  const char *game_name = getenv( "FUSE_ML_GAME" );
  const char *action_keys = getenv( "FUSE_ML_ACTION_KEYS" );
  const char *reward_addr = getenv( "FUSE_ML_REWARD_ADDR" );
  const char *done_addr = getenv( "FUSE_ML_DONE_ADDR" );
  const char *done_value = getenv( "FUSE_ML_DONE_VALUE" );
  unsigned long parsed_value;

  if( fuse_ml_game_name ) {
    libspectrum_free( fuse_ml_game_name );
    fuse_ml_game_name = NULL;
  }

  fuse_ml_game_active = 0;
  fuse_ml_action_count = 0;
  memset( fuse_ml_action_key_counts, 0, sizeof( fuse_ml_action_key_counts ) );
  fuse_ml_reward_enabled = 0;
  fuse_ml_reward_last_valid = 0;
  fuse_ml_done_enabled = 0;

  if( !game_name || !*game_name ) return 0;
  if( !fuse_ml_game_is_manic_miner( game_name ) ) return 0;

  fuse_ml_game_name = utils_safe_strdup( "MANIC_MINER" );
  fuse_ml_game_active = 1;

  if( action_keys && *action_keys ) {
    if( fuse_ml_game_parse_action_keys( action_keys ) ) {
      ui_error( UI_ERROR_ERROR, "Invalid FUSE_ML_ACTION_KEYS: %s", action_keys );
      fuse_ml_game_shutdown();
      return 1;
    }
  } else {
    fuse_ml_game_set_default_actions();
  }

  if( reward_addr && *reward_addr &&
      !fuse_ml_parse_ulong( reward_addr, &parsed_value ) &&
      parsed_value <= 0xffff ) {
    fuse_ml_reward_enabled = 1;
    fuse_ml_reward_addr = (libspectrum_word)parsed_value;
  }

  if( done_addr && *done_addr &&
      !fuse_ml_parse_ulong( done_addr, &parsed_value ) &&
      parsed_value <= 0xffff ) {
    fuse_ml_done_enabled = 1;
    fuse_ml_done_addr = (libspectrum_word)parsed_value;
  }

  fuse_ml_done_value = 0;
  if( done_value && *done_value &&
      !fuse_ml_parse_ulong( done_value, &parsed_value ) ) {
    fuse_ml_done_value = parsed_value & 0xff;
  }

  return 0;
}

void
fuse_ml_game_shutdown( void )
{
  if( fuse_ml_game_name ) {
    libspectrum_free( fuse_ml_game_name );
    fuse_ml_game_name = NULL;
  }

  fuse_ml_game_active = 0;
  fuse_ml_action_count = 0;
  memset( fuse_ml_action_key_counts, 0, sizeof( fuse_ml_action_key_counts ) );
  fuse_ml_reward_enabled = 0;
  fuse_ml_done_enabled = 0;
  fuse_ml_reward_last_valid = 0;
}

int
fuse_ml_game_enabled( void )
{
  return fuse_ml_game_active;
}

int
fuse_ml_game_get_action_keys( unsigned long action, unsigned long *keys,
                              size_t max_keys, size_t *key_count )
{
  size_t count;

  if( !fuse_ml_game_active ) return 1;
  if( action >= fuse_ml_action_count ) return 1;
  if( !keys || !key_count ) return 1;

  count = fuse_ml_action_key_counts[action];
  if( !count || count > max_keys ) return 1;

  memcpy( keys, fuse_ml_action_keys[action], count * sizeof( keys[0] ) );
  *key_count = count;
  return 0;
}

int
fuse_ml_game_evaluate( long *reward, int *done )
{
  unsigned long current_value = 0;

  if( !fuse_ml_game_active ) return 1;

  if( reward ) *reward = 0;
  if( done ) *done = 0;

  if( fuse_ml_reward_enabled ) {
    current_value = readbyte_internal( fuse_ml_reward_addr );

    if( reward && fuse_ml_reward_last_valid ) {
      long delta = (long)current_value - (long)fuse_ml_reward_last;

      if( delta < -128 ) delta += 256;
      if( delta > 128 ) delta -= 256;

      *reward = delta;
    }

    fuse_ml_reward_last = current_value;
    fuse_ml_reward_last_valid = 1;
  }

  if( done && fuse_ml_done_enabled ) {
    unsigned long done_value_now = readbyte_internal( fuse_ml_done_addr );
    *done = ( done_value_now == fuse_ml_done_value ) ? 1 : 0;
  }

  return 0;
}

void
fuse_ml_game_resync( void )
{
  if( !fuse_ml_game_active ) return;

  if( fuse_ml_reward_enabled ) {
    fuse_ml_reward_last = readbyte_internal( fuse_ml_reward_addr );
    fuse_ml_reward_last_valid = 1;
  } else {
    fuse_ml_reward_last_valid = 0;
  }
}

int
fuse_ml_game_info( char *buffer, size_t length )
{
  char reward_address_text[16];
  char done_address_text[16];
  int written;

  if( !buffer || !length ) return 1;

  if( !fuse_ml_game_active ) {
    written = snprintf( buffer, length, "GAME OFF\n" );
    return ( written < 0 || (size_t)written >= length ) ? 1 : 0;
  }

  if( fuse_ml_reward_enabled ) {
    snprintf( reward_address_text, sizeof( reward_address_text ), "0x%04x",
              fuse_ml_reward_addr );
  } else {
    snprintf( reward_address_text, sizeof( reward_address_text ), "-" );
  }

  if( fuse_ml_done_enabled ) {
    snprintf( done_address_text, sizeof( done_address_text ), "0x%04x",
              fuse_ml_done_addr );
  } else {
    snprintf( done_address_text, sizeof( done_address_text ), "-" );
  }

  written = snprintf( buffer, length,
                      "GAME ON %s %lu %s %s %lu\n",
                      fuse_ml_game_name ? fuse_ml_game_name : "UNKNOWN",
                      fuse_ml_action_count,
                      reward_address_text,
                      done_address_text,
                      fuse_ml_done_value );
  return ( written < 0 || (size_t)written >= length ) ? 1 : 0;
}
