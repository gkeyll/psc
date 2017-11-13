
#include "psc_method_private.h"

#include <psc_fields_vpic.h>
#include <psc_particles_vpic.h>
#include <psc_push_particles_vpic.h>

#include <psc_marder.h>

#include <vpic_iface.h>

// ======================================================================
// psc_method "vpic"

// ----------------------------------------------------------------------
// psc_method_vpic_do_setup

static void
psc_method_vpic_do_setup(struct psc_method *method, struct psc *psc)
{
  struct vpic_simulation_info info;
  vpic_simulation_init(&info);

  MPI_Comm comm = psc_comm(psc);
  MPI_Barrier(comm);

  psc->prm.nmax = info.num_step;
  psc->prm.stats_every = info.status_interval;

  struct psc_kind *kinds = calloc(info.n_kinds, sizeof(*kinds));
  for (int m = 0; m < info.n_kinds; m++) {
    kinds[m].q = info.kinds[m].q;
    kinds[m].m = info.kinds[m].m;
    // map "electron" -> "e", "ion"-> "i" to avoid even more confusion with
    // how moments etc are named.
    if (strcmp(info.kinds[m].name, "electron") == 0) {
      kinds[m].name = "e";
    } else if (strcmp(info.kinds[m].name, "ion") == 0) {
      kinds[m].name = "i";
    } else {
      kinds[m].name = info.kinds[m].name;
    }
  }
  psc_set_kinds(psc, info.n_kinds, kinds);
  free(kinds);
  
  psc_marder_set_param_int(psc->marder, "clean_div_e_interval", info.clean_div_e_interval);
  psc_marder_set_param_int(psc->marder, "clean_div_b_interval", info.clean_div_b_interval);
  psc_marder_set_param_int(psc->marder, "sync_shared_interval", info.sync_shared_interval);
  psc_marder_set_param_int(psc->marder, "num_div_e_round", info.num_div_e_round);
  psc_marder_set_param_int(psc->marder, "num_div_b_round", info.num_div_b_round);

  int *np = psc->domain.np;
  mpi_printf(comm, "domain: np = %d x %d x %d\n", np[0], np[1], np[2]);
  //int np[3] = { 4, 1, 1 }; // FIXME, hardcoded, but really hard to get from vpic

  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  assert(size == np[0] * np[1] * np[2]);

  int p[3];
  p[2] = rank / (np[1] * np[0]); rank -= p[2] * (np[1] * np[0]);
  p[1] = rank / np[0]; rank -= p[1] * np[0];
  p[0] = rank;

  double x0[3], x1[3];
  for (int d = 0; d < 3; d++) {
    x0[d] = info.x0[d] - p[d] * (info.x1[d] - info.x0[d]);
    x1[d] = info.x0[d] + (np[d] - p[d]) * (info.x1[d] - info.x0[d]);
  }

  // FIXME, it's also not so obvious that this is going to be the same on all procs
  mprintf("domain: local x0 %g:%g:%g x1 %g:%g:%g\n",
	  info.x0[0], info.x0[1], info.x0[2],
	  info.x1[0], info.x1[1], info.x1[2]);
  mprintf("domain: p %d %d %d\n",
	  p[0], p[1], p[2]);
  mprintf("domain: global x0 %g:%g:%g x1 %g:%g:%g\n",
	  x0[0], x0[1], x0[2],
	  x1[0], x1[1], x1[2]);

  // set size of simulation box to match vpic
  for (int d = 0; d < 3; d++) {
    psc->domain.length[d] = x1[d] - x0[d];
    psc->domain.corner[d] = x0[d];
    psc->domain.gdims[d] = np[d] * info.nx[d];
    psc->domain.np[d] = np[d];
  }

  psc->dt = info.dt;
  mpi_printf(comm, "method_vpic_do_setup: Setting dt = %g\n", psc->dt);

  psc->n_state_fields = VPIC_MFIELDS_N_COMP;
  // having two ghost points wouldn't really hurt, however having no ghost points
  // in the invariant direction does cause trouble.
  // By setting this here, it will override what otherwise happens automatically
  psc->ibn[0] = psc->ibn[1] = psc->ibn[2] = 1;
  mpi_printf(comm, "method_vpic_do_setup: Setting n_state_fields = %d, ibn = [%d,%d,%d]\n",
	     psc->n_state_fields, psc->ibn[0], psc->ibn[1], psc->ibn[2]);

  psc_setup_coeff(psc);
  psc_setup_domain(psc);
}

// ----------------------------------------------------------------------
// psc_method_vpic_setup_partition_and_particles

static void
psc_method_vpic_setup_partition_and_particles(struct psc_method *method, struct psc *psc)
{
  // set particles x^{n+1/2}, p^{n+1/2}
  psc_setup_partition_and_particles(psc);

  // right now, the vpic-internal particles have been initialized by
  // deck.cxx, (while the base particles have potentially been
  // initialized by setup_particles/init_npt).
  //
  // the general logic expects the base particles to be initialized, so
  // we need to copy them over from the vpic particles first
  struct psc_mparticles *mprts_vpic = psc_mparticles_get_as(psc->particles, "vpic", MP_DONT_COPY | MP_DONT_RESIZE);
  psc_mparticles_put_as(mprts_vpic, psc->particles, 0);
}

// ----------------------------------------------------------------------
// psc_method_vpic_setup_fields

static void
psc_method_vpic_setup_fields(struct psc_method *method, struct psc *psc)
{
  // right now, the vpic-internal fields are already initialized, but
  // the general logic expects the base fields to be initialized, so
  // we need to copy them over first
  struct psc_mfields *mflds_vpic = psc_mfields_get_as(psc->flds, "vpic", 0, 0);
  psc_mfields_put_as(mflds_vpic, psc->flds, 0, VPIC_MFIELDS_N_COMP);

  // fields may get initialized twice here -- first in deck.cxx, and
  // then the regular PSC way.
  // FIXME, this is kinda confusing, but not easy to do much about.
  psc_setup_fields(psc);
}

// ----------------------------------------------------------------------
// psc_method_vpic_initialize

static void
psc_method_vpic_initialize(struct psc_method *method, struct psc *psc)
{
  struct psc_mfields *mflds_base = psc->flds;
  struct psc_mparticles *mprts_base = psc->particles;
  struct psc_mfields *mflds = psc_mfields_get_as(mflds_base, "vpic", 0, VPIC_MFIELDS_N_COMP);
  struct psc_mparticles *mprts = psc_mparticles_get_as(mprts_base, "vpic", MP_DONT_COPY);
  
  // Do some consistency checks on user initialized fields

  mpi_printf(psc_comm(psc), "Checking interdomain synchronization\n");
  double err = psc_mfields_synchronize_tang_e_norm_b(mflds);
  mpi_printf(psc_comm(psc), "Error = %g (arb units)\n", err);
  
  mpi_printf(psc_comm(psc), "Checking magnetic field divergence\n");
  psc_mfields_compute_div_b_err(mflds);
  err = psc_mfields_compute_rms_div_b_err(mflds);
  mpi_printf(psc_comm(psc), "RMS error = %e (charge/volume)\n", err);
  psc_mfields_clean_div_b(mflds);
  
  // Load fields not initialized by the user

  mpi_printf(psc_comm(psc), "Initializing radiation damping fields\n");
  psc_mfields_compute_curl_b(mflds);

  mpi_printf(psc_comm(psc), "Initializing bound charge density\n");
  psc_mfields_clear_rhof(mflds);
  psc_mfields_accumulate_rho_p(mflds, mprts);
  psc_mfields_synchronize_rho(mflds);
  psc_mfields_compute_rhob(mflds);

  // Internal sanity checks

  mpi_printf(psc_comm(psc), "Checking electric field divergence\n");
  psc_mfields_compute_div_e_err(mflds);
  err = psc_mfields_compute_rms_div_e_err(mflds);
  mpi_printf(psc_comm(psc), "RMS error = %e (charge/volume)\n", err);
  psc_mfields_clean_div_e(mflds);

  mpi_printf(psc_comm(psc), "Rechecking interdomain synchronization\n");
  err = psc_mfields_synchronize_tang_e_norm_b(mflds);
  mpi_printf(psc_comm(psc), "Error = %e (arb units)\n", err);

  mpi_printf(psc_comm(psc), "Uncentering particles\n");
  psc_push_particles_stagger(psc->push_particles, mprts, mflds);

  psc_mparticles_put_as(mprts, mprts_base, 0);
  psc_mfields_put_as(mflds, mflds_base, 0, VPIC_MFIELDS_N_COMP);

  // First output / stats
  
  mpi_printf(psc_comm(psc), "Performing initial diagnostics.\n");
  vpic_diagnostics();
  psc_method_default_output(method, psc);

  vpic_print_status();
  psc_stats_log(psc);
  psc_print_profiling(psc);
}

// ----------------------------------------------------------------------
// psc_method_vpic_output

static void
psc_method_vpic_output(struct psc_method *method, struct psc *psc)
{
  // FIXME, a hacky place to do this
  vpic_inc_step(psc->timestep);

  vpic_diagnostics();
  
  if (psc->prm.stats_every > 0 && psc->timestep % psc->prm.stats_every == 0) {
    vpic_print_status();
  }
  
  psc_method_default_output(NULL, psc);
}

// ----------------------------------------------------------------------
// psc_method "vpic"

struct psc_method_ops psc_method_ops_vpic = {
  .name                          = "vpic",
  .do_setup                      = psc_method_vpic_do_setup,
  .setup_fields                  = psc_method_vpic_setup_fields,
  .setup_partition_and_particles = psc_method_vpic_setup_partition_and_particles,
  .initialize                    = psc_method_vpic_initialize,
  .output                        = psc_method_vpic_output,
};