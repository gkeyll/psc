
#include "psc_bnd_fields_private.h"

#include "psc.h"
#include "psc_glue.h"
#include <mrc_profile.h>

// FIXME, this is useful at most for testing and maybe should go away

// ======================================================================
// psc_bnd_fields: subclass "none"

struct psc_bnd_fields_ops psc_bnd_fields_none_ops = {
  .name                  = "none",
};
