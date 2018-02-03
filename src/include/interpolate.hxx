
#ifndef INTERPOLATE_HXX
#define INTERPOLATE_HXX

#include "fields.hxx" // for dim_xyz etc, FIXME

// ----------------------------------------------------------------------
// cuda compatibility stuff

#ifndef __CUDACC__
#define __host__
#define __device__
#endif

// ----------------------------------------------------------------------
// get_fint_remainder

template<typename R>
void __host__ __device__ get_fint_remainder(int *lg, R *h, R u)
{
#ifdef  __CUDA_ARCH__
  int l = __float2int_rd(u);
#else
  int l = fint(u);
#endif
  *lg = l;
  *h = u - l;
}

// ----------------------------------------------------------------------
// get_nint_remainder

template<typename R>
static inline void
get_nint_remainder(int *lg1, R *h1, R u)
{
  int l = nint(u);
  *lg1 = l;
  *h1 = l-u;
}

// ======================================================================
// ip_coeff

// ----------------------------------------------------------------------
// ip_coeff_1st

template<typename R>
struct ip_coeff_1st
{
  __host__ __device__ void set(R u)
  {
    R h;
    
    get_fint_remainder(&l, &h, u);
    v0 = 1.f - h;
    v1 = h;
  }

  R v0, v1;
  int l;
};

// ----------------------------------------------------------------------
// ip_coeff_2nd

template<typename R>
struct ip_coeff_2nd
{
  void set(R u)
  {
    get_nint_remainder(&l, &h, u);
    vm = .5f * (.5f+h)*(.5f+h);
    v0 = .75f - h*h;
    vp = .5f * (.5f-h)*(.5f-h);
  }

  R vm, v0, vp, h;
  int l;
};

// ======================================================================
// ip_coeffs

template<typename R, typename OPT_IP>
struct ip_coeffs {};

template<typename R>
struct ip_coeffs<R, opt_ip_1st_ec>
{
  using ip_coeff_t = ip_coeff_1st<R>;
  
  __host__ __device__ void set(R xm)
  {
    g.set(xm);
  }
  
  ip_coeff_t g;
};

template<typename R, typename IP_COEFF>
struct ip_coeffs_std
{
  using ip_coeff_t = IP_COEFF;
  
  __host__ __device__ void set(R xm)
  {
    g.set(xm);
    h.set(xm - R(.5));
  }
  
  ip_coeff_t g;
  ip_coeff_t h;
};

template<typename R>
struct ip_coeffs<R, opt_ip_1st> : ip_coeffs_std<R, ip_coeff_1st<R>> {};

template<typename R>
struct ip_coeffs<R, opt_ip_2nd> : ip_coeffs_std<R, ip_coeff_2nd<R>> {};

// ======================================================================
// InterpolateEM_Helper
//
// empty general class, partially specialized for
// interpolation order and dim
//
// FIXME: the repeated ex/y/z, functions that just call
// cc() should be consolidated.
// It'd also be possible to consolidate the various dim versions based
// on just 1-d interpolation and some fancy template metaprogramming...

template<typename F, typename IP, typename OPT_IP, typename OPT_DIM>
struct InterpolateEM_Helper
{
};

// ======================================================================
// InterpolateEM_Helper: 1st EC

// ----------------------------------------------------------------------
// InterpolateEM_Helper: 1st EC, dim_xyz

template<typename F, typename IP>
struct InterpolateEM_Helper<F, IP, opt_ip_1st_ec, dim_xyz>
{
  using real_t = typename F::real_t;

  static real_t ex(const IP& ip, const F& EM)
  {
    return (ip.cz.g.v0*(ip.cy.g.v0*EM(EX, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l  ) +
			ip.cy.g.v1*EM(EX, ip.cx.g.l  ,ip.cy.g.l+1,ip.cz.g.l  )) +
	    ip.cz.g.v1*(ip.cy.g.v0*EM(EX, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l+1) +
			ip.cy.g.v1*EM(EX, ip.cx.g.l  ,ip.cy.g.l+1,ip.cz.g.l+1)));
  }

  static real_t ey(const IP& ip, const F& EM)
  {
    return (ip.cx.g.v0*(ip.cz.g.v0*EM(EY, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l  ) +
			ip.cz.g.v1*EM(EY, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l+1)) +
	    ip.cx.g.v1*(ip.cz.g.v0*EM(EY, ip.cx.g.l+1,ip.cy.g.l  ,ip.cz.g.l  ) +
			ip.cz.g.v1*EM(EY, ip.cx.g.l+1,ip.cy.g.l  ,ip.cz.g.l+1)));
  }

  static real_t ez(const IP& ip, const F& EM)
  {
    return (ip.cy.g.v0*(ip.cx.g.v0*EM(EZ, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l  ) +
			ip.cx.g.v1*EM(EZ, ip.cx.g.l+1,ip.cy.g.l  ,ip.cz.g.l  )) +
	    ip.cy.g.v1*(ip.cx.g.v0*EM(EZ, ip.cx.g.l  ,ip.cy.g.l+1,ip.cz.g.l  ) +
			ip.cx.g.v1*EM(EZ, ip.cx.g.l+1,ip.cy.g.l+1,ip.cz.g.l  )));
  }

  static real_t hx(const IP& ip, const F& EM)
  {
    return (ip.cx.g.v0*EM(HX, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l  ) +
	    ip.cx.g.v1*EM(HX, ip.cx.g.l+1,ip.cy.g.l  ,ip.cz.g.l  ));
  }

  static real_t hy(const IP& ip, const F& EM)
  {
    return (ip.cy.g.v0*EM(HY, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l  ) +
	    ip.cy.g.v1*EM(HY, ip.cx.g.l  ,ip.cy.g.l+1,ip.cz.g.l  ));	     
  }

  static real_t hz(const IP& ip, const F& EM)
  {
    return (ip.cz.g.v0*EM(HZ, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l  ) +
	    ip.cz.g.v1*EM(HZ, ip.cx.g.l  ,ip.cy.g.l  ,ip.cz.g.l+1));	     
  }
};

// ----------------------------------------------------------------------
// InterpolateEM_Helper: 1st EC, dim_yz

template<typename F, typename IP>
struct InterpolateEM_Helper<F, IP, opt_ip_1st_ec, dim_yz>
{
  using real_t = typename F::real_t;

  __host__ __device__ static real_t ex(const IP& ip, const F& EM)
  {
    return (ip.cz.g.v0*(ip.cy.g.v0*EM(EX, 0,ip.cy.g.l  ,ip.cz.g.l  ) +
			ip.cy.g.v1*EM(EX, 0,ip.cy.g.l+1,ip.cz.g.l  )) +
	    ip.cz.g.v1*(ip.cy.g.v0*EM(EX, 0,ip.cy.g.l  ,ip.cz.g.l+1) +
			ip.cy.g.v1*EM(EX, 0,ip.cy.g.l+1,ip.cz.g.l+1)));
  }

  __host__ __device__ static real_t ey(const IP& ip, const F& EM)
  {
    return (ip.cz.g.v0*EM(EY, 0,ip.cy.g.l  ,ip.cz.g.l  ) +
	    ip.cz.g.v1*EM(EY, 0,ip.cy.g.l  ,ip.cz.g.l+1));
  }

  __host__ __device__ static real_t ez(const IP& ip, const F& EM)
  {
    return (ip.cy.g.v0*EM(EZ, 0,ip.cy.g.l  ,ip.cz.g.l  ) +
	    ip.cy.g.v1*EM(EZ, 0,ip.cy.g.l+1,ip.cz.g.l  ));
  }

  __host__ __device__ static real_t hx(const IP& ip, const F& EM)
  {
    return (EM(HX, 0,ip.cy.g.l  ,ip.cz.g.l  ));
  }

  __host__ __device__ static real_t hy(const IP& ip, const F& EM)
  {
    return (ip.cy.g.v0*EM(HY, 0,ip.cy.g.l  ,ip.cz.g.l  ) +
	    ip.cy.g.v1*EM(HY, 0,ip.cy.g.l+1,ip.cz.g.l  ));
  }

  __host__ __device__ static real_t hz(const IP& ip, const F& EM)
  {
    return (ip.cz.g.v0*EM(HZ, 0,ip.cy.g.l  ,ip.cz.g.l  ) +
	    ip.cz.g.v1*EM(HZ, 0,ip.cy.g.l  ,ip.cz.g.l+1));
  }
};

// ======================================================================
// InterpolateEM_Helper: 1st std

// ----------------------------------------------------------------------
// InterpolateEM_Helper: 1st std, dim_xz

template<typename F, typename IP>
struct InterpolateEM_Helper<F, IP, opt_ip_1st, dim_xz>
{
  using real_t = typename F::real_t;
  using ip_coeff_t = typename IP::ip_coeff_t;

  static real_t cc(const ip_coeff_t& gx, const ip_coeff_t& gy, const ip_coeff_t& gz,
		   const F& EM, int m)
  {
    return (gz.v0*(gx.v0*EM(m, gx.l  ,0,gz.l  ) +
		   gx.v1*EM(m, gx.l+1,0,gz.l  )) +
	    gz.v1*(gx.v0*EM(m, gx.l  ,0,gz.l+1) +
		   gx.v1*EM(m, gx.l+1,0,gz.l+1)));
  }

  static real_t ex(const IP& ip, const F& EM) { return cc(ip.cx.h, ip.cy.g, ip.cz.g, EM, EX); }
  static real_t ey(const IP& ip, const F& EM) { return cc(ip.cx.g, ip.cy.h, ip.cz.g, EM, EY); }
  static real_t ez(const IP& ip, const F& EM) { return cc(ip.cx.g, ip.cy.g, ip.cz.h, EM, EZ); }
  static real_t hx(const IP& ip, const F& EM) { return cc(ip.cx.g, ip.cy.h, ip.cz.h, EM, HX); }
  static real_t hy(const IP& ip, const F& EM) { return cc(ip.cx.h, ip.cy.g, ip.cz.h, EM, HY); }
  static real_t hz(const IP& ip, const F& EM) { return cc(ip.cx.h, ip.cy.h, ip.cz.g, EM, HZ); }
};

// ----------------------------------------------------------------------
// InterpolateEM_Helper: 1st std, dim_yz

template<typename F, typename IP>
struct InterpolateEM_Helper<F, IP, opt_ip_1st, dim_yz>
{
  using real_t = typename F::real_t;
  using ip_coeff_t = typename IP::ip_coeff_t;

  __host__ __device__ static real_t cc(const ip_coeff_t& gx, const ip_coeff_t& gy, const ip_coeff_t& gz,
				       const F& EM, int m)
  {
    return (gz.v0*(gy.v0*EM(m, 0,gy.l  ,gz.l  ) +
		   gy.v1*EM(m, 0,gy.l+1,gz.l  )) +
	    gz.v1*(gy.v0*EM(m, 0,gy.l  ,gz.l+1) +
		   gy.v1*EM(m, 0,gy.l+1,gz.l+1)));
  }

  __host__ __device__ static real_t ex(const IP& ip, const F& EM) { return cc(ip.cx.h, ip.cy.g, ip.cz.g, EM, EX); }
  __host__ __device__ static real_t ey(const IP& ip, const F& EM) { return cc(ip.cx.g, ip.cy.h, ip.cz.g, EM, EY); }
  __host__ __device__ static real_t ez(const IP& ip, const F& EM) { return cc(ip.cx.g, ip.cy.g, ip.cz.h, EM, EZ); }
  __host__ __device__ static real_t hx(const IP& ip, const F& EM) { return cc(ip.cx.g, ip.cy.h, ip.cz.h, EM, HX); }
  __host__ __device__ static real_t hy(const IP& ip, const F& EM) { return cc(ip.cx.h, ip.cy.g, ip.cz.h, EM, HY); }
  __host__ __device__ static real_t hz(const IP& ip, const F& EM) { return cc(ip.cx.h, ip.cy.h, ip.cz.g, EM, HZ); }
};

// ----------------------------------------------------------------------
// InterpolateEM_Helper: dim_1, any interpolation

template<typename F, typename IP, typename OPT_IP>
struct InterpolateEM_Helper<F, IP, OPT_IP, dim_1>
{
  using real_t = typename F::real_t;

  static real_t ex(const IP& ip, const F& EM) { return EM(EX, 0,0,0); }
  static real_t ey(const IP& ip, const F& EM) { return EM(EY, 0,0,0); }
  static real_t ez(const IP& ip, const F& EM) { return EM(EZ, 0,0,0); }
  static real_t hx(const IP& ip, const F& EM) { return EM(HX, 0,0,0); }
  static real_t hy(const IP& ip, const F& EM) { return EM(HY, 0,0,0); }
  static real_t hz(const IP& ip, const F& EM) { return EM(HZ, 0,0,0); }
};

#endif
