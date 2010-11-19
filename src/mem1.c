/*
** 2007 August 14
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This file contains low-level memory allocation drivers for when
** SQLite will use the standard C-library malloc/realloc/free interface
** to obtain the memory it needs.
**
** This file contains implementations of the low-level memory allocation
** routines specified in the sqlite3_mem_methods object.
*/
#include "sqliteInt.h"

/*
** This version of the memory allocator is the default.  It is
** used when no other memory allocator is specified using compile-time
** macros.
*/
#ifdef SQLITE_SYSTEM_MALLOC

#if (!defined(__APPLE__))

#define SQLITE_MALLOC(x) malloc(x)
#define SQLITE_FREE(x) free(x)
#define SQLITE_REALLOC(x,y) realloc((x),(y))


#else


#include <sys/sysctl.h>
#include <malloc/malloc.h>
#include <libkern/OSAtomic.h>

static malloc_zone_t* _sqliteZone_;

#define SQLITE_MALLOC(x) malloc_zone_malloc(_sqliteZone_, (x))
#define SQLITE_FREE(x) malloc_zone_free(_sqliteZone_, (x));
#define SQLITE_REALLOC(x,y) malloc_zone_realloc(_sqliteZone_, (x), (y))

#endif

/*
** Like malloc(), but remember the size of the allocation
** so that we can find it later using sqlite3MemSize().
**
** For this low-level routine, we are guaranteed that nByte>0 because
** cases of nByte<=0 will be intercepted and dealt with by higher level
** routines.
*/
static void *sqlite3MemMalloc(int nByte){
  sqlite3_int64 *p;
  assert( nByte>0 );
  nByte = ROUND8(nByte);
  p = SQLITE_MALLOC( nByte+8 );
  if( p ){
    p[0] = nByte;
    p++;
  }else{
    testcase( sqlite3GlobalConfig.xLog!=0 );
    sqlite3_log(SQLITE_NOMEM, "failed to allocate %u bytes of memory", nByte);
  }
  return (void *)p;
}

/*
** Like free() but works for allocations obtained from sqlite3MemMalloc()
** or sqlite3MemRealloc().
**
** For this low-level routine, we already know that pPrior!=0 since
** cases where pPrior==0 will have been intecepted and dealt with
** by higher-level routines.
*/
static void sqlite3MemFree(void *pPrior){
  sqlite3_int64 *p = (sqlite3_int64*)pPrior;
  assert( pPrior!=0 );
  p--;
  SQLITE_FREE(p);
}

/*
** Report the allocated size of a prior return from xMalloc()
** or xRealloc().
*/
static int sqlite3MemSize(void *pPrior){
  sqlite3_int64 *p;
  if( pPrior==0 ) return 0;
  p = (sqlite3_int64*)pPrior;
  p--;
  return (int)p[0];
}

/*
** Like realloc().  Resize an allocation previously obtained from
** sqlite3MemMalloc().
**
** For this low-level interface, we know that pPrior!=0.  Cases where
** pPrior==0 while have been intercepted by higher-level routine and
** redirected to xMalloc.  Similarly, we know that nByte>0 becauses
** cases where nByte<=0 will have been intercepted by higher-level
** routines and redirected to xFree.
*/
static void *sqlite3MemRealloc(void *pPrior, int nByte){
  sqlite3_int64 *p = (sqlite3_int64*)pPrior;
  assert( pPrior!=0 && nByte>0 );
  assert( nByte==ROUND8(nByte) ); /* EV: R-46199-30249 */
  p--;
  p = SQLITE_REALLOC(p, nByte+8 );
  if( p ){
    p[0] = nByte;
    p++;
  }else{
    testcase( sqlite3GlobalConfig.xLog!=0 );
    sqlite3_log(SQLITE_NOMEM,
      "failed memory resize %u to %u bytes",
      sqlite3MemSize(pPrior), nByte);
  }
  return (void*)p;
}

/*
** Round up a request size to the next valid allocation size.
*/
static int sqlite3MemRoundup(int n){
  return ROUND8(n);
}

/*
** Initialize this module.
*/
static int sqlite3MemInit(void *NotUsed){
#if defined(__APPLE__)
	if (_sqliteZone_) {
		return SQLITE_OK;
	}
	int cpuCount;
    size_t len;
    
    len = sizeof(cpuCount);
    sysctlbyname("hw.ncpu", &cpuCount, &len, NULL, 0);  //  one almost always wants to use "hw.activecpu" for MT decisions, but not here.
	
	if (cpuCount > 1) {
		// defer MT decisions to system malloc
		_sqliteZone_ = malloc_default_zone();
	} else {
		// only 1 core, use our own zone to contention over global locks, 
		// e.g. we have our own dedicated locks
		malloc_zone_t* newzone = malloc_create_zone(4096, 0);
		malloc_set_zone_name(newzone, "Sqlite_Heap");

		bool success;		
		do {
			success = OSAtomicCompareAndSwapPtrBarrier(NULL, newzone, (void * volatile *)&_sqliteZone_);
		} while (!_sqliteZone_);
		
		if (!success) {
			// somebody registered a zone first
			malloc_destroy_zone(newzone);
		}
	}
#endif

  UNUSED_PARAMETER(NotUsed);
  return SQLITE_OK;
}

/*
** Deinitialize this module.
*/
static void sqlite3MemShutdown(void *NotUsed){
#if (0 && defined(__APPLE__))
	if (_sqliteZone_ && (_sqliteZone_ != malloc_default_zone())) {
		malloc_zone_t* oldzone = _sqliteZone_;

		bool success = OSAtomicCompareAndSwapPtrBarrier(oldzone, NULL, (void * volatile *)&_sqliteZone_);
		if (success) {
			malloc_destroy_zone(oldzone);
		}
	}
#endif
	
  UNUSED_PARAMETER(NotUsed);
  return;
}

/*
** This routine is the only routine in this file with external linkage.
**
** Populate the low-level memory allocation function pointers in
** sqlite3GlobalConfig.m with pointers to the routines in this file.
*/
void sqlite3MemSetDefault(void){
  static const sqlite3_mem_methods defaultMethods = {
     sqlite3MemMalloc,
     sqlite3MemFree,
     sqlite3MemRealloc,
     sqlite3MemSize,
     sqlite3MemRoundup,
     sqlite3MemInit,
     sqlite3MemShutdown,
     0
  };
  sqlite3_config(SQLITE_CONFIG_MALLOC, &defaultMethods);
}

#undef SQLITE_MALLOC
#undef SQLITE_FREE
#undef SQLITE_REALLOC

#endif /* SQLITE_SYSTEM_MALLOC */
