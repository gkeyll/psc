
#include "cuda_mparticles.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mrc_profile.h>

struct prof_globals prof_globals; // FIXME

int
prof_register(const char *name, float simd, int flops, int bytes)
{
  return 0;
}

// ----------------------------------------------------------------------
// cuda_mparticles_add_particles_test_1
//
// add 1 particle at the center of each cell, in the "wrong" order in each
// patch (C order, but to get them ordered by block, they need to be reordered
// into Fortran order, a.k.a., this will exercise the initial sorting

static void
set_particle_test_1(struct cuda_mparticles_prt *prt, int n, void *ctx)
{
  const Grid_t& grid = *static_cast<Grid_t*>(ctx);

  Int3 ldims = grid.ldims;
  Vec3<double> dx = grid.dx;

  int k = n % ldims[2];
  n /= ldims[2];
  int j = n % ldims[1];
  n /= ldims[1];
  int i = n;

  prt->xi[0] = dx[0] * (i + .5f);
  prt->xi[1] = dx[1] * (j + .5f);
  prt->xi[2] = dx[2] * (k + .5f);
  prt->pxi[0] = i;
  prt->pxi[1] = j;
  prt->pxi[2] = k;
  prt->kind = 0;
  prt->qni_wni = 1.;
}

void
cuda_mparticles_add_particles_test_1(struct cuda_mparticles *cmprts,
				     const Grid_t& grid,
				     uint *n_prts_by_patch)
{
  Int3 ldims = grid.ldims;

  for (int p = 0; p < grid.patches.size(); p++) {
    n_prts_by_patch[p] = ldims[0] * ldims[1] * ldims[2];
  }

  cmprts->reserve_all(n_prts_by_patch);
  cmprts->resize_all(n_prts_by_patch);
  
  uint off = 0;
  for (int p = 0; p < grid.patches.size(); p++) {
    cmprts->set_particles(n_prts_by_patch[p], off, set_particle_test_1,
			  (void*) &grid);
    off += n_prts_by_patch[p];
  }
}

// ----------------------------------------------------------------------
// get_particles_test

static void
get_particle(struct cuda_mparticles_prt *prt, int n, void *ctx)
{
  printf("prt[%d] xi %g %g %g // pxi %g %g %g // kind %d // qni_wni %g\n",
	 n, prt->xi[0], prt->xi[1], prt->xi[2],
	 prt->pxi[0], prt->pxi[1], prt->pxi[2],
	 prt->kind, prt->qni_wni);
}

void
get_particles_test(struct cuda_mparticles *cmprts, int n_patches,
		   uint *n_prts_by_patch)
{
  uint off = 0;
  for (int p = 0; p < n_patches; p++) {
    cmprts->get_particles(n_prts_by_patch[p], off, get_particle, NULL);
    off += n_prts_by_patch[p];
  }
}

// ----------------------------------------------------------------------
// main

int
main(void)
{
  Grid_t grid{};
  grid.gdims = { 1, 4, 2 };
  grid.ldims = { 1, 4, 2 };
  grid.dx = { 1., 10., 10. };
  grid.fnqs = 1.;
  grid.eta = 1.;
  grid.dt = 1.;

  Grid_t::Patch patch{};
  patch.xb = { 0., 0., 0. };
  grid.patches.push_back(patch);

  grid.kinds.push_back(Grid_t::Kind(-1.,  1., "electron"));
  grid.kinds.push_back(Grid_t::Kind( 1., 25., "ion"));

  Int3 bs = { 1, 1, 1 };
  
  struct cuda_mparticles *cmprts = new cuda_mparticles(grid, bs);

  uint n_prts_by_patch[grid.patches.size()];
  cuda_mparticles_add_particles_test_1(cmprts, grid, n_prts_by_patch);
  printf("added particles\n");
  cmprts->dump_by_patch(n_prts_by_patch);

  cmprts->setup_internals();
  printf("set up internals\n");
  cmprts->dump();

  printf("get_particles_test\n");
  get_particles_test(cmprts, grid.patches.size(), n_prts_by_patch);

  delete cmprts;
}