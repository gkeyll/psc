
#ifndef PSC_PARTICLE_CUDA2_H
#define PSC_PARTICLE_CUDA2_H

#include "psc_particles_private.h"

#include "cuda_wrap.h"

typedef float particle_cuda2_real_t;

#define MPI_PARTICLES_CUDA2_REAL MPI_FLOAT

typedef struct psc_particle_cuda2 {
  particle_cuda2_real_t xi[3];
  particle_cuda2_real_t kind_as_float;
  particle_cuda2_real_t pxi[3];
  particle_cuda2_real_t qni_wni;
} particle_cuda2_t;

struct psc_particles_cuda2 {
  // on host
  float4 *h_xi4, *h_pxi4;
  float4 *h_xi4_alt, *h_pxi4_alt;
  unsigned int *b_idx;
  unsigned int *b_ids;
  unsigned int *b_cnt;
  unsigned int *b_off;

  int n_alloced;
  particle_cuda2_real_t dxi[3];
  int b_mx[3];
  int nr_blocks;
};

#define psc_particles_cuda2(prts) mrc_to_subobj(prts, struct psc_particles_cuda2)

struct psc_mparticles_cuda2 {
  int n_part_total; // total in all patches
  int n_alloced_total;

  particle_cuda2_real_t dxi[3];
  int b_mx[3];
  int bs[3];
  int nr_blocks;
  int nr_blocks_total;

  // on host
  float4 *h_xi4, *h_pxi4;
  float4 *h_xi4_alt, *h_pxi4_alt;
  unsigned int *h_b_off;

  // on device
  float4 *d_xi4, *d_pxi4;
  unsigned int *d_b_off;
};

#define psc_mparticles_cuda2(prts) mrc_to_subobj(prts, struct psc_mparticles_cuda2)

#define particle_cuda2_wni(p) ({				\
      particle_cuda2_real_t rv;					\
      int kind = particle_cuda2_kind(p);			\
      rv = p->qni_wni / ppsc->kinds[kind].q;			\
      rv;							\
    })

CUDA_DEVICE static inline int
particle_cuda2_kind(particle_cuda2_t *prt)
{
#ifdef __CUDACC__
  return __float_as_int(prt->kind_as_float);
#else
  return cuda_float_as_int(prt->kind_as_float);
#endif
}

#define particle_cuda2_qni_wni(prt) ((prt)->qni_wni)

#define particle_cuda2_x(prt) ((prt)->xi[0])
#define particle_cuda2_px(prt) ((prt)->pxi[0])

CUDA_DEVICE static inline int
particle_cuda2_real_fint(particle_cuda2_real_t x)
{
#ifdef __CUDACC__
  return __float2int_rd(x);
#else
  return floorf(x);
#endif
}

CUDA_DEVICE static inline particle_cuda2_real_t
particle_cuda2_real_sqrt(particle_cuda2_real_t x)
{
  return sqrtf(x);
}

CUDA_DEVICE static inline particle_cuda2_real_t
particle_cuda2_real_abs(particle_cuda2_real_t x)
{
  return fabsf(x);
}

#define PARTICLE_CUDA2_LOAD_POS(prt, d_xi4, n) do {			\
    float4 xi4 = d_xi4[n];						\
    (prt).xi[0] = xi4.x;						\
    (prt).xi[1] = xi4.y;						\
    (prt).xi[2] = xi4.z;						\
    (prt).kind_as_float = xi4.w;					\
  } while (0)

#define PARTICLE_CUDA2_LOAD_MOM(prt, d_pxi4, n) do {			\
    float4 pxi4 = d_pxi4[n];						\
    (prt).pxi[0]  = pxi4.x;						\
    (prt).pxi[1]  = pxi4.y;						\
    (prt).pxi[2]  = pxi4.z;						\
    (prt).qni_wni = pxi4.w;						\
  } while (0)

#define PARTICLE_CUDA2_STORE_POS(pp, d_xi4, n) do {			\
    float4 xi4 = { (pp).xi[0], (pp).xi[1], (pp).xi[2], (pp).kind_as_float }; \
    d_xi4[n] = xi4;							\
} while (0)

#define PARTICLE_CUDA2_STORE_MOM(pp, d_pxi4, n) do {			\
    float4 pxi4 = { (pp).pxi[0], (pp).pxi[1], (pp).pxi[2], (pp).qni_wni }; \
    d_pxi4[n] = pxi4;							\
} while (0)


#endif


