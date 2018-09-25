// Copyright 2017, Brown University, Providence, RI.
//
//                         All Rights Reserved
//
// Permission to use, copy, modify, and distribute this software and
// its documentation for any purpose other than its incorporation into a
// commercial product or service is hereby granted without fee, provided
// that the above copyright notice appear in all copies and that both
// that copyright notice and this permission notice appear in supporting
// documentation, and that the name of Brown University not be used in
// advertising or publicity pertaining to distribution of the software
// without specific, written prior permission.
//
// BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR ANY
// PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR
// ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
//
// MGARD: MultiGrid Adaptive Reduction of Data
// Authors: Mark Ainsworth, Ozan Tugluk, Ben Whitney
// Corresponding Author: Ozan Tugluk
//
// version: 0.0.0.1
//
// This file is part of MGARD.
//
// MGARD is distributed under the OSI-approved Apache License, Version 2.0.
// See accompanying file Copyright.txt for details.
//


#include<stdio.h> 
#include<stdlib.h>
#include<math.h>
#include<string.h>

#include "mgard_capi.h" 

#define print_red "\e[31m"

int main(int argc, char *argv[])
{
  int i, j, nrow, ncol;
  double tol_coarse, tol_fine;

  if(argc < 8)
    {
      fprintf (stderr, "%s: Not enough arguments! Usage: %s infile outfile_coarse outfile_fine nrow ncol tolerance\n", argv[0], argv[0]);
      return 1;
    }
  else
    {
      nrow = atoi(argv[4]);
      ncol = atoi(argv[5]);
      tol_coarse  = atof(argv[6]);
      tol_fine  = atof(argv[7]);
    }
  

  FILE * pFile;
  long lSize;
  char * buffer;
  size_t result;

  pFile = fopen ( argv[1] , "rb" );
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  fseek (pFile , 0 , SEEK_END);
  lSize = ftell (pFile);
  rewind (pFile);

  buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}


  fclose (pFile);

  
  double* in_buff = (double*) malloc (sizeof(char)*lSize);

  memcpy (in_buff, buffer, lSize);

   
  unsigned char* mgard_comp_buff_coarse;
  unsigned char* mgard_comp_buff_fine;
  
  int iflag = 1; //0 -> float, 1 -> double
  int out_size_1, out_size_2;

  /* Perform multi-compression*/
  mgard_comp_buff_coarse = mgard_compress(iflag, in_buff, &out_size_1,  nrow,  ncol, &tol_coarse );
  mgard_comp_buff_fine   = mgard_compress(iflag, in_buff, &out_size_2,  nrow,  ncol, &tol_fine );  

  printf ("In size:  %10ld  Out size: %10d  Compression ratio: %10ld \n", lSize, out_size_1, lSize/out_size_1);
  printf ("In size:  %10ld  Out size: %10d  Compression ratio: %10ld \n", lSize, out_size_2, lSize/out_size_2);
 

  FILE *qfile;
  char * outbuffer = ((char*)mgard_comp_buff_coarse);
  qfile = fopen ( argv[2] , "wb" );
  result = fwrite (outbuffer, 1, out_size_1, qfile);
  fclose(qfile);


  outbuffer = ((char*)mgard_comp_buff_fine);
  qfile = fopen ( argv[3] , "wb" );
  result = fwrite (outbuffer, 1, out_size_2, qfile);
  fclose(qfile);


  
  double *mgard_out_buff_coarse;
  double *mgard_out_buff_fine;
 
  mgard_out_buff_coarse = mgard_decompress(iflag, mgard_comp_buff_coarse, out_size_1,  nrow,  ncol); 
  mgard_out_buff_fine   = mgard_decompress(iflag, mgard_comp_buff_fine  , out_size_2,  nrow,  ncol); 


  
  

}
