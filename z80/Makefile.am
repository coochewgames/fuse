## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2003 Philip Kendall
## Copyright (c) 2015-2016 Sergio Baldoví

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk
## Refactoring: rd.mcn@ccgapps.net.au

fuse_SOURCES += \
				z80/execute_z80_command.c \
				z80/execute_z80_opcode.c \
				z80/mnemonics.c \
				z80/parse_z80_operands.c \
				z80/process_z80_opcodes.c \
				z80/read_ops_from_dat_file.c \
				z80/z80_debugger_variables.c \
				z80/z80_opcodes.c \
				z80/z80.c

noinst_HEADERS += \
				z80/execute_z80_command.h \
				z80/execute_z80_opcode.h \
				z80/mnemonics.h \
				z80/parse_z80_operands.h \
				z80/process_z80_opcodes.h \
				z80/read_ops_from_dat_file.h \
				z80/z80_checks.h \
				z80/z80_internals.h \
				z80/z80_macros.h \
				z80/z80_opcodes.h \
				z80/z80.h

EXTRA_DIST += \
			  z80/tests/README \
			  z80/tests/tests.expected \
			  z80/tests/tests.in \
			  z80/opcodes_base.dat \
			  z80/opcodes_cb.dat \
			  z80/opcodes_ddfd.dat \
			  z80/opcodes_ddfdcb.dat \
			  z80/opcodes_ed.dat

## The core tester

noinst_PROGRAMS += z80/coretest

## Coretest needs to have a special version of perform_contend_read* to avoid mass dependencies
z80_coretest_SOURCES = \
					z80/coretest.c \
					z80/process_z80_opcodes.c \
					z80/execute_z80_command.c \
					z80/execute_z80_opcode.c \
					z80/mnemonics.c \
					z80/parse_z80_operands.c \
					z80/read_ops_from_dat_file.c \
					z80/z80_opcodes.c \
					z80/z80.c

z80_coretest_LDADD = \
					logging.o \
					$(GLIB_LIBS) \
					$(LIBSPECTRUM_LIBS)

z80_coretest_CPPFLAGS = $(GLIB_CFLAGS) $(LIBSPECTRUM_CFLAGS) -DCORETEST

test: z80/coretest
	z80/coretest $(srcdir)/z80/tests/tests.in > z80/tests.actual
	cmp z80/tests.actual $(srcdir)/z80/tests/tests.expected

CLEANFILES += \
			  z80/tests.actual \
			  z80/*.o
