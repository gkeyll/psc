
#ifndef GGCM_MHD_BND_PRIVATE_H
#define GGCM_MHD_BND_PRIVATE_H

#include "ggcm_mhd_bnd.h"

struct ggcm_mhd_bnd {
  struct mrc_obj obj;
  struct ggcm_mhd *mhd;
};

struct ggcm_mhd_bnd_ops {
  MRC_SUBCLASS_OPS(struct ggcm_mhd_bnd);
  void (*fill_ghosts)(struct ggcm_mhd_bnd *bnd, struct mrc_fld *fld,
		      int m, float bntim);
};

extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_none;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_sc_ggcm_float;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_sc_ggcm_double;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_sc_float;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_sc_double;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_fc_double;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_sc_double_aos;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_fc_double_aos;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_inoutflow_gkeyll;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_sphere_sc_float;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_sphere_fc_float;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_sphere_sc_double;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_sphere_fc_double;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_sphere_sc_double_aos;
extern struct ggcm_mhd_bnd_ops ggcm_mhd_bnd_ops_sphere_fc_double_aos;

#define ggcm_mhd_bnd_ops(bnd) ((struct ggcm_mhd_bnd_ops *)((bnd)->obj.ops))

// ----------------------------------------------------------------------
// ggcm_mhd_bnd_sphere_map
//
// infrastructure for internal spherical boundaries

struct ggcm_mhd_bnd_sphere_map {
  struct ggcm_mhd *mhd;
  double min_dr;
  double r1;
  double r2;
};

void ggcm_mhd_bnd_sphere_map_find_dr(struct ggcm_mhd_bnd_sphere_map *map, double *dr);
void ggcm_mhd_bnd_sphere_map_find_r1_r2(struct ggcm_mhd_bnd_sphere_map *map,
					double radius, double *p_r1, double *p_r2);


#endif
