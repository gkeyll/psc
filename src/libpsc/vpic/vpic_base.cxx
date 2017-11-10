
#include "vpic_iface.h"
#include "vpic_mfields.h"
#include "vpic_mparticles.h"
#include "vpic_push_particles.h"

#include <vpic.h>

#include <mpi.h>

#include <cassert>

#define mprintf(fmt...) do { int __rank; MPI_Comm_rank(MPI_COMM_WORLD, &__rank); { printf("[%d] ", __rank); printf(fmt); } } while(0)

#define MHERE do { int __rank; MPI_Comm_rank(MPI_COMM_WORLD, &__rank); printf("[%d] HERE: in %s() at %s:%d\n", __rank, __FUNCTION__, __FILE__, __LINE__); } while(0)

extern vpic_simulation *simulation;

// ======================================================================
// vpic_mfields

// ----------------------------------------------------------------------
// vpic_mfields_create

struct vpic_mfields *
vpic_mfields_create()
{
  return new vpic_mfields;
}

// ----------------------------------------------------------------------
// vpic_mfields_ctor_from_simulation

void
vpic_mfields_ctor_from_simulation(struct vpic_mfields *vmflds)
{
  vmflds->field_array = simulation->field_array;
}

// ======================================================================
// vpic_mparticles

// ----------------------------------------------------------------------
// vpic_mparticles_create

struct vpic_mparticles *
vpic_mparticles_create()
{
  return new vpic_mparticles;
}

// ----------------------------------------------------------------------
// vpic_mparticles_ctor_from_simulation

void
vpic_mparticles_ctor_from_simulation(struct vpic_mparticles *vmprts)
{
  vmprts->species_list = simulation->species_list;
}

// ======================================================================
// vpic_push_particles

// ----------------------------------------------------------------------
// vpic_push_particles_create

struct vpic_push_particles *
vpic_push_particles_create()
{
  return new vpic_push_particles;
}

// ----------------------------------------------------------------------
// vpic_push_particles_ctor_from_simulation

void
vpic_push_particles_ctor_from_simulation(struct vpic_push_particles *vpushp)
{
  vpushp->interpolator_array = simulation->interpolator_array;
  vpushp->accumulator_array = simulation->accumulator_array;
}

// ----------------------------------------------------------------------
// vpic_push_particles_push_mprts

void vpic_push_particles_push_mprts(struct vpic_push_particles *vpushp,
				    struct vpic_mparticles *vmprts,
				    struct vpic_mfields *vmflds)
{
  // FIXME, this is kinda too much stuff all in here,
  // so it should be split up, but it'll do for now

  // At this point, fields are at E_0 and B_0 and the particle positions
  // are at r_0 and u_{-1/2}.  Further the mover lists for the particles should
  // empty and all particles should be inside the local computational domain.
  // Advance the particle lists.

  if (vmprts->species_list) {
    vpic_clear_accumulator_array(vpushp);
    vpic_advance_p(vpushp, vmprts);
  }

  // Because the partial position push when injecting aged particles might
  // place those particles onto the guard list (boundary interaction) and
  // because advance_p requires an empty guard list, particle injection must
  // be done after advance_p and before guard list processing. Note:
  // user_particle_injection should be a stub if species_list is empty.

  vpic_emitter();

  if (vmprts->species_list) {
    // This should be after the emission and injection to allow for the
    // possibility of thread parallelizing these operations

    vpic_reduce_accumulator_array(vpushp);
  }
  // At this point, most particle positions are at r_1 and u_{1/2}. Particles
  // that had boundary interactions are now on the guard list. Process the
  // guard lists. Particles that absorbed are added to rhob (using a corrected
  // local accumulation).

  vpic_boundary_p(vpushp, vmprts, vmflds);

  // At this point, all particle positions are at r_1 and u_{1/2}, the
  // guard lists are empty and the accumulators on each processor are current.
  // Convert the accumulators into currents.

  vpic_mfields_clear_jf(vmflds);
  if (vmprts->species_list) {
    vpic_push_particles_unload_accumulator_array(vpushp, vmflds);
  }
  vpic_mfields_synchronize_jf(vmflds);

  // At this point, the particle currents are known at jf_{1/2}.
  // Let the user add their own current contributions. It is the users
  // responsibility to insure injected currents are consistent across domains.
  // It is also the users responsibility to update rhob according to
  // rhob_1 = rhob_0 + div juser_{1/2} (corrected local accumulation) if
  // the user wants electric field divergence cleaning to work.

  vpic_current_injection();
}

// ----------------------------------------------------------------------
// vpic_push_particles_prep

void vpic_push_particles_prep(struct vpic_push_particles *vpushp,
			      struct vpic_mparticles *vmprts, struct vpic_mfields *vmflds)
{
  if (vmprts->species_list) {
    vpic_push_particles_load_interpolator_array(vpushp, vmflds);
  }
}

// ======================================================================

// ----------------------------------------------------------------------
// vpic_base_init

void
vpic_base_init(struct vpic_simulation_info *info)
{
  if (simulation) {
    return;
  }
  
  //  boot_services( &argc, &argv );
  {
    int argc = 0;
    char *_argv[] = {}, **argv = _argv;
    int *pargc = &argc;
    char ***pargv = &argv;
    // Start up the checkpointing service.  This should be first.
    
    boot_checkpt( pargc, pargv );
    
    serial.boot( pargc, pargv );
    thread.boot( pargc, pargv );
    
    // Boot up the communications layer
    // See note above about thread-core-affinity
    
    boot_mp( pargc, pargv );
    
    mp_barrier();
    _boot_timestamp = 0;
    _boot_timestamp = uptime();
  }

  if( world_rank==0 ) log_printf( "*** Initializing\n" );
  simulation = new vpic_simulation;
  simulation->initialize( 0, NULL );

  info->num_step = simulation->num_step;
  info->clean_div_e_interval = simulation->clean_div_e_interval;
  info->clean_div_b_interval = simulation->clean_div_b_interval;
  info->sync_shared_interval = simulation->sync_shared_interval;
}

void vpic_inc_step(int step)
{
  simulation->grid->step++;
  assert(simulation->grid->step == step);
}


