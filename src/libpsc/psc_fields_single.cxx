
#include "psc.h"
#include "psc_fields_as_single.h"
#include "psc_fields_c.h"
#include "fields.hxx"

#include <mrc_params.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "psc_fields_inc.h"

using Fields = Fields3d<fields_t>;
using FieldsC = Fields3d<fields_c_t>;

// ======================================================================
// convert to c

static void
psc_mfields_single_copy_from_c(struct psc_mfields *mflds, struct psc_mfields *mflds_c,
			       int mb, int me)
{
  for (int p = 0; p < mflds->nr_patches; p++) {
    fields_t flds = fields_t_mflds(mflds, p);
    fields_c_t flds_c = fields_c_t_mflds(mflds_c, p);
    Fields F(flds);
    FieldsC F_c(flds_c);
    for (int m = mb; m < me; m++) {
      for (int jz = flds.ib[2]; jz < flds.ib[2] + flds.im[2]; jz++) {
	for (int jy = flds.ib[1]; jy < flds.ib[1] + flds.im[1]; jy++) {
	  for (int jx = flds.ib[0]; jx < flds.ib[0] + flds.im[0]; jx++) {
	    F(m, jx,jy,jz) = F_c(m, jx,jy,jz);
	  }
	}
      }
    }
  }
}

void
psc_mfields_single_copy_to_c(struct psc_mfields *mflds, struct psc_mfields *mflds_c,
			     int mb, int me)
{
  for (int p = 0; p < mflds->nr_patches; p++) {
    fields_t flds = fields_t_mflds(mflds, p);
    fields_c_t flds_c = fields_c_t_mflds(mflds_c, p);
    Fields F(flds);
    FieldsC F_c(flds_c);
    for (int m = mb; m < me; m++) {
      for (int jz = flds.ib[2]; jz < flds.ib[2] + flds.im[2]; jz++) {
	for (int jy = flds.ib[1]; jy < flds.ib[1] + flds.im[1]; jy++) {
	  for (int jx = flds.ib[0]; jx < flds.ib[0] + flds.im[0]; jx++) {
	    F_c(m, jx,jy,jz) = F(m, jx,jy,jz);
	  }
	}
      }
    }
  }
}

static struct mrc_obj_method psc_mfields_single_methods[] = {
  MRC_OBJ_METHOD("copy_to_c",   psc_mfields_single_copy_to_c),
  MRC_OBJ_METHOD("copy_from_c", psc_mfields_single_copy_from_c),
  {}
};

#include "psc_fields_common.cxx"
