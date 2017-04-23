#ifndef __MALC_TEST_ALLOCATOR__
#define __MALC_TEST_ALLOCATOR__

#include <string.h>
#include <bl/base/allocator.h>
#include <bl/base/integer.h>
#include <bl/base/to_type_containing.h>

/*----------------------------------------------------------------------------*/
typedef struct alloc_data {
  alloc_tbl alloc;
  uword     alloc_count;
  uword     realloc_count;
  uword     dealloc_count;
  uword     alloc_succeeds;
  uword     realloc_succeeds;
  u8*       buffer;
}
alloc_data;
/*----------------------------------------------------------------------------*/
static void* test_alloc_func (size_t bytes, alloc_tbl const* invoker)
{
  alloc_data* ad = to_type_containing (invoker, alloc, alloc_data);
  ++ad->alloc_count;
  ad->alloc_succeeds = ad->alloc_succeeds ? ad->alloc_succeeds - 1 : 0;
  return ad->alloc_succeeds ? ad->buffer : nullptr;
}
/*----------------------------------------------------------------------------*/
static void* test_realloc_func(
  void* mem, size_t new_size, alloc_tbl const* invoker
  )
{
  alloc_data* ad = to_type_containing (invoker, alloc, alloc_data);
  ++ad->realloc_count;
  ad->realloc_succeeds = ad->realloc_succeeds ? ad->realloc_succeeds - 1 : 0;
  return ad->realloc_succeeds ? ad->buffer : nullptr;
}
/*----------------------------------------------------------------------------*/
static void test_dealloc_func (void const* mem, alloc_tbl const* invoker)
{
  alloc_data* ad = to_type_containing (invoker, alloc, alloc_data);
  ++ad->dealloc_count;
}
/*----------------------------------------------------------------------------*/
static inline alloc_data alloc_data_init (u8* buffer)
{
  alloc_data ad;
  ad.alloc.alloc      = test_alloc_func;
  ad.alloc.realloc    = test_realloc_func;
  ad.alloc.dealloc    = test_dealloc_func;
  ad.alloc_count      = 0;
  ad.realloc_count    = 0;
  ad.dealloc_count    = 0;
  ad.alloc_succeeds   = (uword) -1;
  ad.realloc_succeeds = (uword) -1;
  ad.buffer           = buffer;
  return ad;
}
/*----------------------------------------------------------------------------*/

#endif