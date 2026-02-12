/* ml_game_adapter.h: game-specific ML adapter helpers for Fuse
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

#ifndef FUSE_ML_GAME_ADAPTER_H
#define FUSE_ML_GAME_ADAPTER_H

#include <stdlib.h>

#define FUSE_ML_GAME_MAX_KEYS_PER_ACTION 4

int fuse_ml_game_configure_from_env( void );
void fuse_ml_game_shutdown( void );
int fuse_ml_game_enabled( void );
int fuse_ml_game_get_action_keys( unsigned long action, unsigned long *keys,
                                  size_t max_keys, size_t *key_count );
int fuse_ml_game_evaluate( long *reward, int *done );
void fuse_ml_game_resync( void );
int fuse_ml_game_info( char *buffer, size_t length );

#endif			/* #ifndef FUSE_ML_GAME_ADAPTER_H */
