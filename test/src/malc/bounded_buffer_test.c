#include <string.h>

#include <bl/cmocka_pre.h>

#include <malc/bounded_buffer.h>
#include <malc/test_allocator.h>

#include <bl/base/default_allocator.h>
#include <bl/base/integer.h>
#include <bl/base/utility.h>
#include <bl/base/static_integer_math.h>

/*----------------------------------------------------------------------------*/
typedef struct boundedb_context {
  alloc_tbl alloc;
  boundedb  b;
}
boundedb_context;
/*----------------------------------------------------------------------------*/
static int boundedb_test_setup (void **state)
{
  static boundedb_context c;
  memset (&c, 0, sizeof c);
  c.alloc = get_default_alloc();
  boundedb_init (&c.b);
  *state  = (void*) &c;
  return 0;
}
/*----------------------------------------------------------------------------*/
static int boundedb_test_teardown (void **state)
{
  boundedb_context* c = (boundedb_context*) *state;
  boundedb_destroy (&c->b, &c->alloc);
  return 0;
}
/*----------------------------------------------------------------------------*/
static void boundedb_alloc_dealloc_test (void **state)
{
  const uword slots     = 4;
  const uword slot_size = 32;

  boundedb_context* c = (boundedb_context*) *state;
  bl_err err = boundedb_reset(
    &c->b, &c->alloc, slot_size * slots, slot_size, 1, false
    );
  assert_int_equal (err.bl, bl_ok);

  for (uword round = 0; round < 2; ++round) {
    u8* mem[slots];
    for (uword i = 0; i < slots; ++i) {
      err = boundedb_alloc (&c->b, &mem[i], 1);
      assert_int_equal (err.bl, bl_ok);
    }
    u8* dummy;
    err = boundedb_alloc (&c->b, &dummy, 1);
    assert_int_equal (err.bl, bl_alloc);
    for (uword i = 0; i < slots; ++i) {
      boundedb_dealloc (&c->b, mem[i], 1);
    }
  }
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown(
    boundedb_alloc_dealloc_test, boundedb_test_setup, boundedb_test_teardown
    ),
};
/*----------------------------------------------------------------------------*/
int bounded_buffer_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
