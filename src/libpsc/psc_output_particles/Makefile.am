
AM_CFLAGS = -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libmrc/include

noinst_LTLIBRARIES = libsubdir.la

libsubdir_la_SOURCES = \
	psc_output_particles.c \
	psc_output_particles_none.c \
	psc_output_particles_ascii.c

if HAVE_HDF5
libsubdir_la_SOURCES += \
	psc_output_particles_hdf5.c \
	psc_output_particles_hdf5_single.c \
	psc_output_particles_hdf5_double.c \
	psc_output_particles_custom_binary.c \
	psc_output_particles_xdmf_compact.c \
	psc_output_particles_xdmf_spread.c \
	psc_output_particles_xdmf_common.c
endif

