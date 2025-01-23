#ifndef CORETEST_H
#define CORETEST_H

#include "libspectrum.h"

void perform_contend_read( libspectrum_word address, libspectrum_dword time );
void perform_contend_read_no_mreq( libspectrum_word address, libspectrum_dword time );
void perform_contend_write_no_mreq( libspectrum_word address, libspectrum_dword time );

#endif // CORETEST_H