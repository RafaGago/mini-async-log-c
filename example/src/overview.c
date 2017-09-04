#include <stdio.h>

#include <bl/base/default_allocator.h>
#include <bl/base/thread.h>

#include <malc/malc.h>
#include <malc/destinations/file.h>
#include <malc/destinations/stdouterr.h>

malc* ilog = nullptr;
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_logger_instance()
{
  return ilog;
}
/*----------------------------------------------------------------------------*/
void dtor_using_free (void* context, malc_ref const* refs, uword refs_count)
{
  for (uword i = 0; i < refs_count; ++i) {
    free ((void*) refs[i].ref);
  }
}
/*----------------------------------------------------------------------------*/
int log_thread (void* ctx)
{
  bl_err err;

  /* int */
  log_error (err, "10: {}", 10);
  log_error (err, "10 using \"03\" specifier: {03}", 10);
  log_error (err, "10 using \"+03\" specifier: {+03}", 10);
  log_error (err, "10 using \"03x\" specifier: {03x}", 10);
  log_error (err, "10 using \"03x\" specifier: {03X}", 10);
  log_error(
    err,
    "10 using \"0Nx\" specifier. N = number of nibbles: {0Nx}",
    (u32) 10
    );
  log_error(
    err,
    "10 using \"0Wx\" specifier. W = max number of int digits: {0W}",
    (u32) 10
    );

  /* floating point */
  log_error (err, "1.: {}", 1.);
  log_error (err, "1. using \".3\" specifier: {.3}", 1.);
  log_error (err, "1. using \"012.3\" specifier: {012.3}", 1.);
  log_error (err, "1. using \"+012.3\" specifier: {+012.3}", 1.);
  log_error (err, "1. using \"-012.3\" specifier: {-012.3}", 1.);
  log_error (err, "1. using \"e\" specifier: {e}", 1.);
  log_error (err, "1. using \"g\" specifier: {g}", 1.);
  log_error (err, "1. using \"a\" specifier: {a}", 1.);
  log_error (err, "1. using \"E\" specifier: {E}", 1.);
  log_error (err, "1. using \"G\" specifier: {G}", 1.);
  log_error (err, "1. using \"A\" specifier: {A}", 1.);

  /* bytes */
  u8 const mem[] = { 10, 11, 12, 13 };
  log_error (err, "[10,11,12,13] by value: {}", logmemcpy (mem, sizeof mem));

  u8* dmem = malloc (sizeof mem);
  assert (dmem);
  memcpy (dmem, mem, sizeof mem);
  log_error(
    err,
    "[10,11,12,13] by ref: {}",
    logmemref (dmem, sizeof mem),
    logrefdtor (dtor_using_free, nullptr)
    );

  /* strings */
  u8 const str[] = "a demo string";
  log_error (err, "a string by value: {}", logstrcpy (str, sizeof str - 1));

  u8* dstr = malloc (sizeof str);
  assert (dstr);
  memcpy (dstr, str, sizeof str);
  log_error(
    err,
    "a string by ref: {}",
    logstrref (dstr, sizeof str - 1),
    logrefdtor (dtor_using_free, nullptr)
    );

  log_error (err, "a literal: {}", loglit (1 ? "literal one" : "literal two"));

  /* severities */
  log_debug (err, "this is not seen on stdout (sev > debug)");

  /* if */
  log_error_if (err, 1, "conditional debug line");

  /* brace escape */
  log_error (err, "brace escape only requires to skip the open brace: {{}");

  (void) malc_terminate (ilog, false); /* terminating the logger. Will force the
                                          event loop to exit */
  return 0;
}
/*----------------------------------------------------------------------------*/
int add_configure_destinations (void)
{
  u32    stdouterr_id;
  u32    file_id;
  bl_err err;

  /* destination register */
  err = malc_add_destination (ilog, &stdouterr_id, &malc_stdouterr_dst_tbl);
  if (err) {
    fprintf (stderr, "Error creating the stdout/stderr destination\n");
    return err;
  }
  err = malc_add_destination (ilog, &file_id, &malc_file_dst_tbl);
  if (err) {
    fprintf (stderr, "Error creating the file destination\n");
    return err;
  }
  /* from here and below it's just configuration when the defaults are not
  suitable */

  /* destination generic cfg, setting log severities */
  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time = 0;
  dcfg.show_timestamp       = true;
  dcfg.show_severity        = true;
  dcfg.severity_file_path   = nullptr;

  dcfg.severity = malc_sev_warning;
  err = malc_set_destination_cfg (ilog, &dcfg, stdouterr_id);
  if (err) {
    fprintf (stderr, "Unable to set generic stdout/stderr destination config\n");
    return err;
  }
  dcfg.severity = malc_sev_debug;
  err = malc_set_destination_cfg (ilog, &dcfg, file_id);
  if (err) {
    fprintf (stderr, "Unable to set generic file destination config\n");
    return err;
  }
  /* destination specific cfg. */
  malc_stdouterr_dst* stdouterr;
  err = malc_get_destination_instance (ilog, (void**) &stdouterr, stdouterr_id);
  if (err) {
    fprintf (stderr, "bug when retrieving the stdout/stderr destination\n");
    return err;
  }
  malc_file_dst* file;
  err = malc_get_destination_instance (ilog, (void**) &file, file_id);
  if (err) {
    fprintf (stderr, "bug when retrieving the stdout/stderr destination\n");
    return err;
  }
  /* severities equal and above error will be output on stderr (just for demo
  purposes, as it's the default) */
  err = malc_stdouterr_set_stderr_severity (stdouterr, malc_sev_error);
  if (err) {
    fprintf(
      stderr, "Unable to set specific stdout/stderr destination config\n"
      );
    return err;
  }
  malc_file_cfg fcfg;
  err = malc_file_get_cfg (file, &fcfg);
  if (err) {
    fprintf (stderr, "Unable to get specific file destination config\n");
    return err;
  }
  fcfg.prefix = "malc-overview";
  fcfg.suffix = ".log";
  err = malc_file_set_cfg (file, &fcfg);
  if (err) {
    fprintf (stderr, "Unable to set specific file destination config\n");
  }
  return err;
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  bl_err              err;
  alloc_tbl           alloc = get_default_alloc(); /* Using malloc and free */
  /* logger allocation/initialization */
  ilog = bl_alloc (&alloc,  malc_get_size());
  if (!ilog) {
    fprintf (stderr, "Unable to allocate memory for the malc instance\n");
    return bl_alloc;
  }
  err = malc_create (ilog, &alloc);
  if (err) {
    fprintf (stderr, "Error creating the malc instance\n");
    goto dealloc;
  }
  err = add_configure_destinations();
  if (err) {
    goto destroy;
  }
  /* logger startup */
  malc_cfg cfg;
  err = malc_get_cfg (ilog, &cfg);
  if (err) {
    fprintf (stderr, "bug when retrieving the logger configuration\n");
    goto destroy;
  }
  cfg.consumer.start_own_thread = false;
  err = malc_init (ilog, &cfg);
  if (err) {
    fprintf (stderr, "unable to start logger\n");
    goto destroy;
  }
  /* threads can start logging */
  bl_thread t;
  err = bl_thread_init (&t, log_thread, nullptr);
  if (err) {
    fprintf (stderr, "unable to start a log thread\n");
    goto destroy;
  }

  do {
    err = malc_run_consume_task (ilog, 10000);
  }
  while (!err || err == bl_nothing_to_do);
  err = bl_ok;

destroy:
  (void) malc_destroy (ilog);
dealloc:
  bl_dealloc (&alloc, ilog);
  return err;
}
/*----------------------------------------------------------------------------*/
