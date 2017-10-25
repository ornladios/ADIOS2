#include <mpi.h>
#include <cstdio>

#if defined(MPI_VERSION) && defined(MPI_SUBVERSION)
const char mpiver_str[] = { 'I', 'N',
                            'F', 'O',
                            ':', 'M',
                            'P', 'I',
                            '-', 'V',
                            'E', 'R',
                            '[', ('0' + MPI_VERSION),
                            '.', ('0' + MPI_SUBVERSION),
                            ']', '\0' };
#endif

int main(int argc, char* argv[])
{
#if defined(MPI_VERSION) && defined(MPI_SUBVERSION)
  std::puts(mpiver_str);
#endif
  MPI_Init(&argc, &argv);
  MPI_Finalize();
}
