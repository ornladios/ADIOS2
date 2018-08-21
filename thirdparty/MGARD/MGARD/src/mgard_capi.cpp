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


#include "mgard.h"

extern "C" unsigned char *mgard_compress(int itype_flag,  void  *data, int &out_size, int nrow, int ncol, void* tol_in)
{ 

  assert (ncol > 3);
  assert (nrow >= 1);

  if(itype_flag == 0)
    {
      float *v   = static_cast<float*>(data);
      float *tol = static_cast<float*>(tol_in);
      assert (*tol >= 1e-8);
      
      unsigned char* mgard_compressed_ptr;

      //mgard_compressed_ptr = mgard::refactor_qz_float(nrow, ncol, v, out_size, *tol);

      return mgard_compressed_ptr;
    }
  else if(itype_flag == 1)
    {
      double *v   = static_cast<double*>(data);
      double *tol = static_cast<double*>(tol_in);
      assert (*tol >= 1e-8);
      
      unsigned char* mgard_compressed_ptr;

      mgard_compressed_ptr = mgard::refactor_qz(nrow, ncol, v, out_size, *tol);

      return mgard_compressed_ptr;
    }
  else
    {
      std::cerr <<"MGARD: Unknown data type, assuming 32-bit floats...\n";
      const float *v = static_cast<const float*>(data);
      float *tol = static_cast<float*>(tol_in);
      assert (*tol >= 1e-8);
      
      unsigned char* mgard_compressed_ptr;

      //mgard_compressed_ptr = mgard::refactor_qz_float(nrow, ncol, v, out_size, *tol);

      return mgard_compressed_ptr;
    }
  
}



extern "C" void *mgard_decompress(int itype_flag,  unsigned char *data, int data_len, int nrow, int ncol)
{
  assert (ncol > 3);
  assert (nrow >= 1);

  if(itype_flag == 1)
    {
      return mgard::recompose_udq(nrow, ncol, data, data_len);
    }
  else
    {
      if(itype_flag != 0)
        {
          std::cerr <<"MGARD: Unknown data type, assuming 32-bit floats...\n";
        }

      //return mgard::recompose_udq_float(nrow, ncol, v, out_size, *tol);

      return nullptr;
    }
}
