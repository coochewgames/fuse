/* ml_bridge.h: machine-learning control bridge for Fuse
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

#ifndef FUSE_ML_BRIDGE_H
#define FUSE_ML_BRIDGE_H

int fuse_ml_configure_from_env( void );
int fuse_ml_mode_enabled( void );
int fuse_ml_visual_mode_enabled( void );
int fuse_ml_init_socket( void );
int fuse_ml_loop( void );
void fuse_ml_shutdown( void );

#endif			/* #ifndef FUSE_ML_BRIDGE_H */
