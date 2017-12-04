
#ifndef VPIC_GRID_BASE_H
#define VPIC_GRID_BASE_H

#include <vpic.h>

// ======================================================================
// VpicGridBase

struct VpicGridBase : grid_t
{
  static VpicGridBase* create()
  {
    return static_cast<VpicGridBase*>(::new_grid());
  }

  void setup(double dx_[3], double dt_, double cvac_, double eps0_)
  {
    dx = dx_[0]; dy = dx_[1]; dz = dx_[2];
    dt = dt_;
    cvac = cvac_;
    eps0 = eps0_;
  }

  void partition_periodic_box(double xl[3], double xh[3], int gdims[3], int np[3])
  {
    ::partition_periodic_box(this, xl[0], xl[1], xl[2], xh[0], xh[1], xh[2],
			     gdims[0], gdims[1], gdims[2], np[0], np[1], np[2]);
  }

  void set_fbc(int boundary, int fbc) { ::set_fbc(this, boundary, fbc); }
  void set_pbc(int boundary, int pbc) { ::set_pbc(this, boundary, pbc); }

  void mp_size_recv_buffer(int port, int size) { ::mp_size_recv_buffer(mp, port, size); }
  void mp_size_send_buffer(int port, int size) { ::mp_size_send_buffer(mp, port, size); }
  void mp_begin_recv(int port, int size, int src, int tag) { ::mp_begin_recv(mp, port, size, src, tag); }
  void mp_end_recv(int port) { ::mp_end_recv(mp, port); }
  void mp_begin_send(int port, int size, int dst, int tag) { ::mp_begin_send(mp, port, size, dst, tag); }
  void mp_end_send(int port) { ::mp_end_send(mp, port); }
  
  void* size_send_port(int i, int j, int k, int sz) { return ::size_send_port(i, j, k, sz, this); }
  void begin_send_port(int i, int j, int k, int sz) { ::begin_send_port(i, j, k, sz, this); }
  void end_send_port(int i, int j, int k) { ::end_send_port(i, j, k, this); }
  void begin_recv_port(int i, int j, int k, int sz) { ::begin_recv_port(i, j, k, sz, this); }
  void* end_recv_port(int i, int j, int k) { return ::end_recv_port(i, j, k, this); }
};

#endif