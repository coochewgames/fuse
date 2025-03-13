#ifndef MEMORY_CONFIG_H
#define MEMORY_CONFIG_H

/* A memory page will be 1 << (this many) bytes in size
   ie 12 => 4 KB, 13 => 8 KB, 14 => 16 KB
   Note: we now rely on 2KB page size so this should no longer be changed
   without a full review of all relevant code and changes will be required to
   match
 */
#define MEMORY_PAGE_SIZE_LOGARITHM 11

/* The actual size of a memory page */
#define MEMORY_PAGE_SIZE ( 1 << MEMORY_PAGE_SIZE_LOGARITHM )

/* The mask to use to select the bits within a page */
#define MEMORY_PAGE_SIZE_MASK ( MEMORY_PAGE_SIZE - 1 )

/* The number of memory pages in 64K
   This calculation is equivalent to 2^16 / MEMORY_PAGE_SIZE */
#define MEMORY_PAGES_IN_64K ( 1 << ( 16 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 16K */
#define MEMORY_PAGES_IN_16K ( 1 << ( 14 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 8K */
#define MEMORY_PAGES_IN_8K ( 1 << ( 13 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 4K */
#define MEMORY_PAGES_IN_4K ( 1 << ( 12 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 2K */
#define MEMORY_PAGES_IN_2K ( 1 << ( 11 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 14K */
#define MEMORY_PAGES_IN_14K ( MEMORY_PAGES_IN_16K - MEMORY_PAGES_IN_2K )

/* The number of memory pages in 12K */
#define MEMORY_PAGES_IN_12K ( MEMORY_PAGES_IN_16K - MEMORY_PAGES_IN_4K )

/* The number of 16Kb RAM pages we support: 1040 Kb needed for the Pentagon 1024 */
#define SPECTRUM_RAM_PAGES 65

/* The maximum number of 16Kb ROMs we support */
#define SPECTRUM_ROM_PAGES 4


#endif