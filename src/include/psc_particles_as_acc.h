
#ifndef PSC_PARTICLES_AS_ACC_H
#define PSC_PARTICLES_AS_ACC_H

#include "psc_particles_acc.h"

typedef particle_acc_real_t particle_real_t;
typedef particle_acc_t particle_t;

#define particles_get_one           particles_acc_get_one
#define particles_realloc           particles_acc_realloc
#define particle_qni_div_mni        particle_acc_qni_div_mni
#define particle_qni_wni            particle_acc_qni_wni
#define particle_qni                particle_acc_qni
#define particle_mni                particle_acc_mni
#define particle_wni                particle_acc_wni
#define particle_kind               particle_acc_kind
#define particle_x                  particle_acc_x
#define particle_px                 particle_acc_px
#define particle_get_relative_pos   particle_acc_get_relative_pos
#define particle_real_nint          particle_acc_real_nint
#define particle_real_fint          particle_acc_real_fint
#define particle_real_abs           particle_acc_real_abs
#define particle_real_sqrt          particle_acc_real_sqrt

#define MPI_PARTICLES_REAL          MPI_PARTICLES_ACC_REAL
#define PARTICLE_TYPE               "acc"

#define PSC_PARTICLES_AS_ACC 1

#endif

