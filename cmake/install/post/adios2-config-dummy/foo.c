#ifdef WITH_ADIOS2
#include <adios2_c.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif
#endif

#include "foo.h"

void foo(void)
{
#ifdef WITH_ADIOS2
#ifdef ADIOS2_HAVE_MPI
    adios2_adios *ctx = adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);
#else
    adios2_adios *ctx = adios2_init(adios2_debug_mode_on);
#endif
#endif
}
