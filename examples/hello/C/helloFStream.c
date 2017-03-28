/*
 * helloFStream.c  Test for C API version of helloFStream
 *  Created on: Nov 2, 2016
 *      Author: wfg
 */

#include "../../../include/ADIOS_C.h"

void main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // populate values
  double *myNumbers;
  myNumbers = (double *)malloc(10 * sizeof(double));

  int i;
  for (i = 0; i < 10; ++i)
    myNumbers[i] = i + 1;

  // start ADIOS
  ADIOS *adiosC, adiosC2;
  const char group[] = "Types";
  const char group2[] = "Types2";

  adiosC2 = adios_init_debug("fstream.xml", MPI_COMM_WORLD); // debug  mode

  adios_create_group(adiosC, "groupCFD");
  adios_create_variable(adiosC, "groupCFD", "temperature");
  adios_create_group(adiosC2, "groupFEM", "temperature");
  ///
  adios_open(adiosC, group, "helloVector.txt", "w"); // open group Types
                                                     // associated with file
                                                     // "helloVector.txt" for
                                                     // writing
  adios_open(adiosC, group2, "Vector.txt", "w"); // open group Types associated
                                                 // with file "helloVector.txt"
                                                 // for writing
  adios_write(adiosC, group, "Types", myNumbers);
  adios_close(adiosC, group);

  free(myNumbers);
}
