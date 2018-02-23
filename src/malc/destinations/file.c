#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <bl/base/integer_math.h>
#include <bl/base/utility.h>
#include <bl/base/dynamic_string.h>
#include <bl/base/ringbuffer.h>
#include <bl/base/integer_printf_format.h>

#include <malc/malc.h>
#include <malc/destinations/file.h>

define_ringb_funcs (past_files, char*);
/*----------------------------------------------------------------------------*/
struct malc_file_dst {
  FILE*            f;
  alloc_tbl const* alloc;
  bool             time_based_name;
  bool             can_remove_old_data_on_full_disk;
  uword            name_seq_num;
  dstr             prefix;
  dstr             suffix;
  uword            file_size;
  uword            max_file_size;
  uword            max_log_files;
  ringb            files;
};
/*----------------------------------------------------------------------------*/
static bl_err malc_file_dst_init (void* instance, alloc_tbl const* alloc)
{
  malc_file_dst* d = (malc_file_dst*) instance;
  memset (d, 0, sizeof *d);
  dstr_init (&d->prefix, alloc);
  dstr_init (&d->suffix, alloc);
  d->alloc = alloc;
  d->time_based_name = true;
  d->can_remove_old_data_on_full_disk = false;
  bl_err err = dstr_set_lit (&d->suffix, ".log");
  if (err.bl) {
    return err;
  }
  err = past_files_init (&d->files, 1, alloc);
  if (err.bl) {
    dstr_destroy (&d->suffix);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
static void malc_file_dst_terminate (void* instance)
{
  malc_file_dst* d = (malc_file_dst*) instance;
  if (d->f) {
    fclose (d->f);
    d->f = nullptr;
  }
  while (past_files_size (&d->files)) {
    bl_dealloc (d->alloc, *past_files_at_head (&d->files));
    past_files_drop_head (&d->files);
  }
  past_files_destroy (&d->files, d->alloc);
  dstr_destroy (&d->prefix);
  dstr_destroy (&d->suffix);
}
/*----------------------------------------------------------------------------*/
static bl_err malc_file_dst_open_new_file (malc_file_dst* d)
{
  dstr name = dstr_init_rv (d->alloc);
  bl_assert (d->file_size == 0);

  bl_err err = dstr_set_capacity(
      &name, dstr_len (&d->prefix) + 1 + 16 + 1 + 16 + dstr_len (&d->suffix)
      );
  if (err.bl) {
      return err;
  }
  if (d->time_based_name) {
    /* no dstr error check: it has already the required space allocated */
    (void) dstr_set_o (&name, &d->prefix);
    (void) dstr_append_va(
      &name,
      "_%016"FMT_X64"_%016"FMT_X64,
      bl_get_sysclock_tstamp(),
      bl_get_tstamp()
      );
    (void) dstr_append_o (&name, &d->suffix);
  }
  else {
    /* find a filename that doesn't exist */
    do {
      if (d->f) {
        fclose (d->f);
        d->f = nullptr;
      }
      /* no dstr error check: it has already the required space allocated */
      (void) dstr_set_o (&name, &d->prefix);
      (void) dstr_append_va (&name, "_%"FMT_UWORD, d->name_seq_num++);
      (void) dstr_append_o (&name, &d->suffix);
      d->f = fopen (dstr_get (&name), "r");
    }
    while (d->f);
  }
  char* fname = dstr_steal_ownership (&name);
  d->f = fopen (fname, "w");
  if (!d->f) {
    err.bl  = (bl_err_uint) bl_error;
    err.sys = (bl_err_uint) errno;
    bl_dealloc (d->alloc, fname);
    return err;
  }
  past_files_insert_tail (&d->files, &fname);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static void malc_file_dst_drop_last_file (malc_file_dst* d, bool do_remove)
{
  if (likely (past_files_size (&d->files))) {
    char* file = *past_files_at_head (&d->files);
    if (do_remove) {
      remove (file);
    }
    bl_dealloc (d->alloc, file);
    past_files_drop_head (&d->files);
  }
}
/*----------------------------------------------------------------------------*/
#define LIT_fwrite(fd, lit) fwrite (lit, 1, lit_len (lit), fd)
#define ENOSPC_ERRSTR "<corrupted: ENOSPC>\n"
/*----------------------------------------------------------------------------*/
static bl_err malc_fwrite (malc_file_dst* d, char const* s, uword slen)
{
  static const uword max_retries = 2;
  uword i;

  for (i = 0; i < max_retries; ++i) {
    uword bytes   = fwrite (s, sizeof (char), slen, d->f);
    d->file_size += bytes;

    if (bytes != slen) {
      if (errno == ENOSPC && d->can_remove_old_data_on_full_disk) {
        uword size = past_files_size (&d->files);
        if (size == 1) {
          /* removing the currently open file */
          fclose (d->f);
          d->f         = nullptr;
          d->file_size = 0;
          malc_file_dst_drop_last_file (d, true);
          bl_err err   = malc_file_dst_open_new_file (d);
          if (err.bl) {
            return err;
          }
        }
        else if (size != 0) {
          /* removing some old file */
          malc_file_dst_drop_last_file (d, true);
          bytes = LIT_fwrite (d->f, ENOSPC_ERRSTR);
          d->file_size += bytes;
          if (bytes != lit_len (ENOSPC_ERRSTR)) {
            return bl_mkerr (bl_error);
          }
        }
        continue;
      }
      else {
        /*handle other type of errors?*/
        return bl_mkerr_sys (bl_error, errno);
      }
    }
    break;
  }
  return bl_mkerr (i < max_retries ? bl_ok : bl_error);
}
/*----------------------------------------------------------------------------*/
static bl_err malc_file_dst_write(
    void* instance, tstamp now, uword sev_val, malc_log_strings const* strs
    )
{
  malc_file_dst* d = (malc_file_dst*) instance;

  uword bytes = strs->tstamp_len + strs->sev_len + strs->text_len;
  bytes      += lit_len ("\n");

  if (d->max_file_size != 0 && d->file_size + bytes >= d->max_file_size) {
    if (d->f) {
      fclose (d->f);
      d->f = nullptr;
      d->file_size = 0;
    }
    if (d->max_log_files != 0) {
      if (past_files_size (&d->files) >= d->max_log_files) {
        malc_file_dst_drop_last_file (d, true);
      }
    }
    else {
      malc_file_dst_drop_last_file (d, false);
    }
  }
  bl_err err;
  if (!d->f) {
    err = malc_file_dst_open_new_file (d);
    if (err.bl) {
      return err;
    }
  }
  err = malc_fwrite (d, strs->tstamp, strs->tstamp_len);
  if (err.bl) {
    return err;
  }
  err = malc_fwrite (d, strs->sev, strs->sev_len);
  if (err.bl) {
    return err;
  }
  malc_fwrite (d, strs->text, strs->text_len);
  if (err.bl) {
    return err;
  }
  return malc_fwrite (d, "\n", lit_len ("\n"));
}
/*----------------------------------------------------------------------------*/
static bl_err malc_file_dst_flush (void* instance)
{
  malc_file_dst* d = (malc_file_dst*) instance;
  if (d->f) {
    return bl_mkerr_sys (fflush (d->f) == 0 ? bl_ok : bl_error, errno);
  }
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT const struct malc_dst malc_file_dst_tbl = {
  sizeof (malc_file_dst), /*size_of*/
  &malc_file_dst_init,
  &malc_file_dst_terminate,
  &malc_file_dst_flush,
  nullptr, /* idle task */
  &malc_file_dst_write
};
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_file_get_cfg (malc_file_dst* d, malc_file_cfg* cfg)
{
  if (!d || !cfg) {
    return bl_mkerr (bl_invalid);
  }
  cfg->prefix = dstr_get (&d->prefix);
  cfg->suffix = dstr_get (&d->suffix);
  cfg->max_file_size = d->max_file_size;
  cfg->max_log_files = d->max_log_files;
  cfg->time_based_name = d->time_based_name;
  cfg->can_remove_old_data_on_full_disk = d->can_remove_old_data_on_full_disk;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_file_set_cfg(
  malc_file_dst* d, malc_file_cfg const* cfg
  )
{
  if (!d || !cfg) {
    return bl_mkerr (bl_invalid);
  }
  if (past_files_size (&d->files)) {
    return bl_mkerr (bl_preconditions);
  }
  past_files_destroy (&d->files, d->alloc);
  d->max_file_size = cfg->max_file_size;
  d->max_log_files = cfg->max_file_size ? cfg->max_log_files : 0;
  d->time_based_name = cfg->time_based_name;
  d->can_remove_old_data_on_full_disk = cfg->can_remove_old_data_on_full_disk;
  bl_err err = dstr_set (&d->prefix, cfg->prefix);
  if (err.bl) {
    return err;
  }
  err = dstr_set (&d->suffix, cfg->suffix);
  if (err.bl) {
    return err;
  }
  uword pfcount = d->max_log_files ? round_next_pow2_u (d->max_log_files) : 1;
  return past_files_init (&d->files, pfcount, d->alloc);
}
/*----------------------------------------------------------------------------*/
