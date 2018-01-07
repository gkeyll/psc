
#include <mrc_io.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// ======================================================================
// psc_mparticles "single" / "double" / "c"

// ----------------------------------------------------------------------
// psc_mparticles_sub_setup_patch

static void
PFX(setup_patch)(struct psc_mparticles *mprts, int p)
{
  struct psc_mparticles_sub *sub = psc_mparticles_sub(mprts);
  struct PFX(patch) *patch = &sub->patch[p];

  PARTICLE_BUF(ctor)(&patch->buf);

  for (int d = 0; d < 3; d++) {
    patch->b_mx[d] = ppsc->patch[p].ldims[d];
    patch->b_dxi[d] = 1.f / ppsc->patch[p].dx[d];
  }
#if PSC_PARTICLES_AS_SINGLE
  patch->nr_blocks = patch->b_mx[0] * patch->b_mx[1] * patch->b_mx[2];
  patch->b_cnt = (unsigned int *) calloc(patch->nr_blocks + 1, sizeof(*patch->b_cnt));
#endif

#if PSC_PARTICLES_AS_SINGLE_BY_BLOCK
  patch->nr_blocks = patch->b_mx[0] * patch->b_mx[1] * patch->b_mx[2];
  patch->b_cnt = (unsigned int *) calloc(patch->nr_blocks + 1, sizeof(*patch->b_cnt));
  patch->b_off = (unsigned int *) calloc(patch->nr_blocks + 2, sizeof(*patch->b_off));
#endif
}

// ----------------------------------------------------------------------
// psc_mparticles_sub_destroy_patch

static void
PFX(destroy_patch)(struct psc_mparticles *mprts, int p)
{
  struct psc_mparticles_sub *sub = psc_mparticles_sub(mprts);
  struct PFX(patch) *patch = &sub->patch[p];

  // need to free structures created in ::patch_setup and ::patch_reserve
  PARTICLE_BUF(dtor)(&patch->buf);

#if PSC_PARTICLES_AS_SINGLE
  free(patch->prt_array_alt);
  free(patch->b_idx);
  free(patch->b_ids);
  free(patch->b_cnt);
#endif

#if PSC_PARTICLES_AS_SINGLE_BY_BLOCK
  free(patch->prt_array_alt);
  free(patch->b_idx);
  free(patch->b_ids);
  free(patch->b_cnt);
  free(patch->b_off);
#endif
}

// ----------------------------------------------------------------------
// psc_mparticles_sub_setup

static void
PFX(setup)(struct psc_mparticles *mprts)
{
  struct psc_mparticles_sub *sub = psc_mparticles_sub(mprts);

  psc_mparticles_setup_super(mprts);
#if PSC_PARTICLES_AS_SINGLE
  sub->patch = (struct psc_mparticles_single_patch *) calloc(mprts->nr_patches, sizeof(*sub->patch));
#elif PSC_PARTICLES_AS_DOUBLE
  sub->patch = (struct psc_mparticles_double_patch *) calloc(mprts->nr_patches, sizeof(*sub->patch));
#elif PSC_PARTICLES_AS_FORTRAN
  sub->patch = (struct psc_mparticles_fortran_patch *) calloc(mprts->nr_patches, sizeof(*sub->patch));
#elif PSC_PARTICLES_AS_SINGLE_BY_BLOCK
  sub->patch = (struct psc_mparticles_single_by_block_patch *) calloc(mprts->nr_patches, sizeof(*sub->patch));
#else
  sub->patch = (struct psc_mparticles_patch *) calloc(mprts->nr_patches, sizeof(*sub->patch));
#endif

  for (int p = 0; p < mprts->nr_patches; p++) {
    PFX(setup_patch)(mprts, p);
  }
}

// ----------------------------------------------------------------------
// psc_mparticls_sub_write/read
  
#if (PSC_PARTICLES_AS_DOUBLE || PSC_PARTICLES_AS_SINGLE) && HAVE_LIBHDF5_HL

// FIXME. This is a rather bad break of proper layering, HDF5 should be all
// mrc_io business. OTOH, it could be called flexibility...

#include <hdf5.h>
#include <hdf5_hl.h>

#define H5_CHK(ierr) assert(ierr >= 0)
#define CE assert(ierr == 0)

// ----------------------------------------------------------------------
// psc_mparticles_sub_write

static void
PFX(write)(struct psc_mparticles *mprts, struct mrc_io *io)
{
  int ierr;
  assert(sizeof(particle_t) / sizeof(particle_real_t) == 8);

  long h5_file;
  mrc_io_get_h5_file(io, &h5_file);

  hid_t group = H5Gopen(h5_file, mrc_io_obj_path(io, mprts), H5P_DEFAULT); H5_CHK(group);
  for (int p = 0; p < mprts->nr_patches; p++) {
    particle_range_t prts = particle_range_mprts(mprts, p);
    char pname[10]; sprintf(pname, "p%d", p);
    hid_t pgroup = H5Gcreate(group, pname, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); H5_CHK(pgroup);
    int n_prts = particle_range_size(prts);
    ierr = H5LTset_attribute_int(pgroup, ".", "n_prts", &n_prts, 1); CE;
    if (n_prts > 0) {
      // in a rather ugly way, we write the int "kind" member as a float / double
      hsize_t hdims[2];
      hdims[0] = n_prts;
      hdims[1] = 8;
#if PSC_PARTICLES_AS_DOUBLE
      ierr = H5LTmake_dataset_double(pgroup, "data", 2, hdims,
				    (double *) particle_iter_deref(prts.begin)); CE;
#elif PSC_PARTICLES_AS_SINGLE
      ierr = H5LTmake_dataset_float(pgroup, "data", 2, hdims,
				    (float *) particle_iter_deref(prts.begin)); CE;
#else
      assert(0);
#endif
    }
    ierr = H5Gclose(pgroup); CE;
  }
  ierr = H5Gclose(group); CE;
}

// ----------------------------------------------------------------------
// psc_mparticles_sub_read

static void
PFX(read)(struct psc_mparticles *mprts, struct mrc_io *io)
{
  int ierr;
  psc_mparticles_read_super(mprts, io);

  PFX(setup)(mprts);
  
  long h5_file;
  mrc_io_get_h5_file(io, &h5_file);
  hid_t group = H5Gopen(h5_file, mrc_io_obj_path(io, mprts), H5P_DEFAULT); H5_CHK(group);

  for (int p = 0; p < mprts->nr_patches; p++) {
    particle_range_t prts = particle_range_mprts(mprts, p);
    char pname[10]; sprintf(pname, "p%d", p);
    hid_t pgroup = H5Gopen(group, pname, H5P_DEFAULT); H5_CHK(pgroup);
    int n_prts;
    ierr = H5LTget_attribute_int(pgroup, ".", "n_prts", &n_prts); CE;
    PFX(patch_reserve)(mprts, p, n_prts);
    
    if (n_prts > 0) {
#if PSC_PARTICLES_AS_SINGLE
      ierr = H5LTread_dataset_float(pgroup, "data",
				    (float *) particle_iter_deref(prts.begin)); CE;
#elif PSC_PARTICLES_AS_DOUBLE
      ierr = H5LTread_dataset_double(pgroup, "data",
				    (double *) particle_iter_deref(prts.begin)); CE;
#else
      assert(0);
#endif
    }
    ierr = H5Gclose(pgroup); CE;
  }
  ierr = H5Gclose(group); CE;
}

#else

static void
PFX(write)(struct psc_mparticles *mprts, struct mrc_io *io)
{
  assert(0);
}

static void
PFX(read)(struct psc_mparticles *mprts, struct mrc_io *io)
{
  assert(0);
}

#endif

static void
PFX(destroy)(struct psc_mparticles *mprts)
{
  struct psc_mparticles_sub *sub = psc_mparticles_sub(mprts);

  for (int p = 0; p < mprts->nr_patches; p++) {
    PFX(destroy_patch)(mprts, p);
  }
  free(sub->patch);
}

static void
PFX(reserve_all)(struct psc_mparticles *mprts, int *n_prts_by_patch)
{
  for (int p = 0; p < mprts->nr_patches; p++) {
    PFX(patch_reserve)(mprts, p, n_prts_by_patch[p]);
  }
}

static void
PFX(resize_all)(struct psc_mparticles *mprts, int *n_prts_by_patch)
{
  struct psc_mparticles_sub *sub = psc_mparticles_sub(mprts);

  for (int p = 0; p < mprts->nr_patches; p++) {
    struct PFX(patch) *patch = &sub->patch[p];
    PARTICLE_BUF(resize)(&patch->buf, n_prts_by_patch[p]);
  }
}

static void
PFX(get_size_all)(struct psc_mparticles *mprts, int *n_prts_by_patch)
{
  struct psc_mparticles_sub *sub = psc_mparticles_sub(mprts);

  for (int p = 0; p < mprts->nr_patches; p++) {
    struct PFX(patch) *patch = &sub->patch[p];
    n_prts_by_patch[p] = PARTICLE_BUF(size)(&patch->buf);
  }
}

static unsigned int
PFX(get_nr_particles)(struct psc_mparticles *mprts)
{
  struct psc_mparticles_sub *sub = psc_mparticles_sub(mprts);

  int n_prts = 0;
  for (int p = 0; p < mprts->nr_patches; p++) {
    struct PFX(patch) *patch = &sub->patch[p];
    n_prts += PARTICLE_BUF(size)(&patch->buf);
  }
  return n_prts;
}

#if PSC_PARTICLES_AS_SINGLE

static void
PFX(inject)(struct psc_mparticles *mprts, int p,
	    const struct psc_particle_inject *new_prt)
{
  int kind = new_prt->kind;

  struct psc_patch *patch = &ppsc->patch[p];
  for (int d = 0; d < 3; d++) {
    assert(new_prt->x[d] >= patch->xb[d]);
    assert(new_prt->x[d] <= patch->xb[d] + patch->ldims[d] * patch->dx[d]);
  }
  
  float dVi = 1.f / (patch->dx[0] * patch->dx[1] * patch->dx[2]);

  particle_t prt;
  prt.xi      = new_prt->x[0] - patch->xb[0];
  prt.yi      = new_prt->x[1] - patch->xb[1];
  prt.zi      = new_prt->x[2] - patch->xb[2];
  prt.pxi     = new_prt->u[0];
  prt.pyi     = new_prt->u[1];
  prt.pzi     = new_prt->u[2];
  prt.qni_wni = new_prt->w * ppsc->kinds[kind].q * dVi;
  prt.kind    = kind;
  
  mparticles_patch_push_back(mprts, p, prt);
}

#endif

// ----------------------------------------------------------------------
// psc_mparticles_ops

struct psc_mparticles_ops PFX(ops) = {
  .name                    = PARTICLE_TYPE,
  .size                    = sizeof(struct psc_mparticles_sub),
  .methods                 = PFX(methods),
  .setup                   = PFX(setup),
  .destroy                 = PFX(destroy),
  .write                   = PFX(write),
  .read                    = PFX(read),
  .reserve_all             = PFX(reserve_all),
  .resize_all              = PFX(resize_all),
  .get_size_all            = PFX(get_size_all),
  .get_nr_particles        = PFX(get_nr_particles),
#if PSC_PARTICLES_AS_SINGLE
  .inject                  = PFX(inject),
#endif
};
