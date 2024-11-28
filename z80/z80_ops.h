/*
    z80_ops.h: Some commonly used z80 things as macros

    Copyright (c) 1999-2011 Philip Kendall
    Copyright (c) 2015 Stuart Brady

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
#ifndef FUSE_Z80_OPS_H
#define FUSE_Z80_OPS_H

#define DIDAKTIK80_UNPAGE_ADDR 0x1700

#define DISCIPLE_PAGE_ADDR1 0x0001
#define DISCIPLE_PAGE_ADDR2 0x0008
#define DISCIPLE_PAGE_ADDR3 0x0066
#define DISCIPLE_PAGE_ADDR4 0x028e

#define USOURCE_TOGGLE_ADDR 0x2bae
#define MULTIFACE_SETIC8_ADDR 0x0066

#define IF1_PAGE_ADDR1 0x0008
#define IF1_PAGE_ADDR2 0x1708
#define IF1_UNPAGE_ADDR 0x0700

#define DIVIDE_AUTOMAP_ADDR_MASK 0xff00
#define DIVIDE_AUTOMAP_ADDR 0x3d00
#define DIVIDE_UNPAGE_ADDR_MASK 0xfff8
#define DIVIDE_UNPAGE_ADDR 0x1ff8
#define DIVIDE_PAGE_ADDR1 0x0000
#define DIVIDE_PAGE_ADDR2 0x0008
#define DIVIDE_PAGE_ADDR3 0x0038
#define DIVIDE_PAGE_ADDR4 0x0066
#define DIVIDE_PAGE_ADDR5 0x04c6
#define DIVIDE_PAGE_ADDR6 0x0562

#define DIVMMC_AUTOMAP_ADDR_MASK 0xff00
#define DIVMMC_AUTOMAP_ADDR 0x3d00

#define SPECTRANET_PAGE_ADDR1 0x0008
#define SPECTRANET_PAGE_ADDR_MASK 0xfff8
#define SPECTRANET_PAGE_ADDR2 0x3ff8

#endif