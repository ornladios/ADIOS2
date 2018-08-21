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
#include "mgard_nuni.h"

namespace mgard
{


unsigned char *
refactor_qz (int nrow, int ncol, const double *u, int &outsize, double tol)
{

  std::vector<double> row_vec (ncol);
  std::vector<double> col_vec (nrow);
  std::vector<double> v(u, u+nrow*ncol), work(nrow * ncol);

  double norm = mgard_common::max_norm(v);

  
  if (mgard::is_2kplus1 (nrow)
      && mgard::is_2kplus1 (ncol)) // input is (2^q + 1) x (2^p + 1)
    {
      int nlevel;
      mgard::set_number_of_levels (nrow, ncol, nlevel);
      tol /= float (nlevel + 1);      
      
      int l_target = nlevel - 1;
      mgard::refactor (nrow, ncol, l_target, v.data(), work, row_vec, col_vec);
      work.clear ();
      row_vec.clear ();
      col_vec.clear ();

      int size_ratio = sizeof (double) / sizeof (int);
      std::vector<int> qv (nrow * ncol + size_ratio);


      mgard::quantize_2D_iterleave (nrow, ncol, v.data(), qv, norm, tol);
      
      std::vector<unsigned char> out_data;
      mgard::compress_memory (qv.data (), sizeof (int) * qv.size (), out_data);
      outsize = out_data.size ();
      unsigned char *buffer = (unsigned char *)malloc (outsize);
      std::copy (out_data.begin (), out_data.end (), buffer);
      return buffer;
    }
  else
    {
      std::vector<double> coords_x, coords_y;
      int nlevel_x = std::log2(ncol-1);
      int nc = std::pow(2, nlevel_x ) + 1; //ncol new

      int nlevel_y = std::log2(nrow-1);
      int nr = std::pow(2, nlevel_y ) + 1; //nrow new

      int nlevel = std::min(nlevel_x, nlevel_y);
      tol /= nlevel + 1;

      int l_target = nlevel-1;


      mgard_gen::prep_2D(nr, nc, nrow, ncol, l_target, v.data(),  work, coords_x, coords_y, row_vec, col_vec);

      mgard_gen::refactor_2D(nr, nc, nrow, ncol, l_target, v.data(),  work, coords_x, coords_y, row_vec, col_vec);

      work.clear ();
      col_vec.clear ();
      row_vec.clear ();

      int size_ratio = sizeof (double) / sizeof (int);
      std::vector<int> qv (nrow * ncol + size_ratio);

      tol /= nlevel + 1;
      mgard::quantize_2D_iterleave (nrow, ncol, v.data (), qv, norm, tol);

      std::vector<unsigned char> out_data;
      mgard::compress_memory (qv.data (), sizeof (int) * qv.size (), out_data);

      outsize = out_data.size ();
      unsigned char *buffer = (unsigned char *)malloc (outsize);
      std::copy (out_data.begin (), out_data.end (), buffer);
      return buffer;
    }
}  


double* recompose_udq(int nrow, int ncol, unsigned char *data, int data_len)
{
  int size_ratio = sizeof(double)/sizeof(int);

  if (mgard::is_2kplus1 (nrow)
      && mgard::is_2kplus1 (ncol)) // input is (2^q + 1) x (2^p + 1)
    {
      int ncol_new = ncol;
      int nrow_new = nrow;
      
      int nlevel_new;
      mgard::set_number_of_levels (nrow_new, ncol_new, nlevel_new);
      int l_target = nlevel_new-1;

      
      std::vector<int> out_data(nrow_new*ncol_new + size_ratio);

      mgard::decompress_memory(data, data_len, out_data.data(), out_data.size()*sizeof(int)); // decompress input buffer

      
      double *v = (double *)malloc (nrow_new*ncol_new*sizeof(double));

      mgard::dequantize_2D_iterleave(nrow_new, ncol_new, v, out_data) ;
      out_data.clear();
      
      std::vector<double> row_vec(ncol_new);
      std::vector<double> col_vec(nrow_new);
      std::vector<double> work(nrow_new*ncol_new);

      mgard::recompose(nrow_new, ncol_new, l_target, v,  work, row_vec, col_vec);
      
      return v;

    }
  else
    {
      std::vector<double> coords_x, coords_y;
      
      int nlevel_x = std::log2(ncol-1);
      int nc = std::pow(2, nlevel_x ) + 1; //ncol new

      int nlevel_y = std::log2(nrow-1);
      int nr = std::pow(2, nlevel_y ) + 1; //nrow new

      int nlevel = std::min(nlevel_x, nlevel_y);

      int l_target = nlevel-1;
      
      std::vector<int> out_data(nrow*ncol + size_ratio);

      mgard::decompress_memory(data, data_len, out_data.data(), out_data.size()*sizeof(int)); // decompress input buffer

      double *v = (double *)malloc (nrow*ncol*sizeof(double));

      mgard::dequantize_2D_iterleave(nrow, ncol, v, out_data) ;
      
      std::vector<double> row_vec(ncol);
      std::vector<double> col_vec(nrow);
      std::vector<double> work(nrow*ncol);

      mgard_gen::recompose_2D(nr, nc, nrow, ncol, l_target, v,  work, coords_x, coords_y, row_vec, col_vec);
      mgard_gen::postp_2D(nr, nc, nrow, ncol, l_target, v,  work, coords_x, coords_y, row_vec, col_vec);
      //      std::cout << "Recomposing done!!!" << "\n";

      return v;
    }
}

  
inline int
get_index (const int ncol, const int i, const int j)
{
  return ncol * i + j;
}

bool
is_2kplus1 (double num)
{
  float frac_part, f_level, int_part;
  if (num == 1) return 1;
  
  f_level = std::log2 (num - 1);
  frac_part = modff (f_level, &int_part);

  if (frac_part == 0)
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

int
parse_cmdl (int argc, char **argv, int &nrow, int &ncol, double &tol,
            std::string &in_file)
{
  if (argc >= 5)
    {
      in_file = argv[1];
      nrow = strtol ((argv[2]), NULL, 0); // number of rows
      ncol = strtol ((argv[3]), NULL, 0); // number of columns
      tol = strtod ((argv[4]), 0);        // error tolerance

      assert (in_file.size () != 0);
      assert (ncol > 3);
      assert (nrow >= 1);
      assert (tol >= 1e-8);

      struct stat file_stats;
      int flag = stat (in_file.c_str (), &file_stats);

      if (flag != 0) // can't stat file somehow
        {
          throw std::runtime_error (
              "Cannot stat input file! Nothing to be done, exiting...");
        }

      return 1;
    }
  else
    {
      std::cerr << "Usage: " << argv[0] << " inputfile nrow ncol tol"
                << "\n";
      throw std::runtime_error ("Too few arguments, exiting...");
    }
}

void
mass_matrix_multiply (const int l, std::vector<double> &v)
{

  int stride = std::pow (2, l);
  double temp1, temp2;
  double fac = 0.5;
  // Mass matrix times nodal value-vec
  temp1 = v.front (); // save u(0) for later use
  v.front () = fac * (2.0 * temp1 + v.at (stride));
  for (auto it = v.begin () + stride; it < v.end () - stride; it += stride)
    {
      temp2 = *it;
      *it = fac * (temp1 + 4 * temp2 + *(it + stride));
      temp1 = temp2; // save u(n) for later use
    }
  v.back () = fac * (2 * v.back () + temp1);
}

void
solve_tridiag_M (const int l, std::vector<double> &v)
{

  //  int my_level = nlevel - l;
  int stride = std::pow (2, l); // current stride

  double am, bm;

  am = 2.0; // first element of upper diagonal U.

  bm = 1.0 / am;

  int nlevel = static_cast<int> (std::log2 (v.size () - 1));
  int n = std::pow (2, nlevel - l) + 1;
  std::vector<double> coeff (n);
  int counter = 1;
  coeff.front () = am;

  // forward sweep
  for (auto it = std::begin (v) + stride; it < std::end (v) - stride;
       it += stride)
    {
      *(it) -= *(it - stride) / am;

      am = 4.0 - bm;
      bm = 1.0 / am;

      coeff.at (counter) = am;
      ++counter;
    }
  am = 2.0 - bm; // a_n = 2 - b_(n-1)

  auto it = v.end () - stride - 1;
  v.back () -= (*it) * bm; // last element

  coeff.at (counter) = am;

  // backward sweep

  v.back () /= am;
  --counter;

  for (auto it = v.rbegin () + stride; it <= v.rend (); it += stride)
    {

      *(it) = (*(it) - *(it - stride)) / coeff.at (counter);
      --counter;
      bm = 4.0 - am; // maybe assign 1/am -> bm?
      am = 1.0 / bm;
    }
}

void
restrict (const int l, std::vector<double> &v)
{
  int stride = std::pow (2, l);
  int Pstride = stride / 2;

  // calculate the result of restriction
  auto it = v.begin () + Pstride;
  v.front () += 0.5 * (*it); // first element
  for (auto it = std::begin (v) + stride; it <= std::end (v) - stride;
       it += stride)
    {
      *(it) += 0.5 * (*(it - Pstride) + *(it + Pstride));
    }
  it = v.end () - Pstride - 1;
  v.back () += 0.5 * (*it); // last element
}

void
interpolate_from_level_nMl (const int l, std::vector<double> &v)
{

  int stride = std::pow (2, l);
  int Pstride = stride / 2;

  for (auto it = std::begin (v) + stride; it < std::end (v); it += stride)
    {
      *(it - Pstride) = 0.5 * (*(it - stride) + *it);
    }
}

void
print_level_2D (const int nrow, const int ncol, const int l, double *v)
{

  int stride = std::pow (2, l);

  for (int irow = 0; irow < nrow; irow += stride)
    {
      std::cout << "\n";
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          std::cout << v[get_index (ncol, irow, jcol)] << "\t";
        }
      std::cout << "\n";
    }
}

void
write_level_2D (const int nrow, const int ncol, const int l, double *v,
                std::ofstream &outfile)
{
  int stride = std::pow (2, l);
  //  int nrow = std::pow(2, nlevel_row) + 1;
  // int ncol = std::pow(2, nlevel_col) + 1;

  for (int irow = 0; irow < nrow; irow += stride)
    {
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          outfile.write (
              reinterpret_cast<char *> (&v[get_index (ncol, irow, jcol)]),
              sizeof (double));
        }
    }
}

void
write_level_2D_exc (const int nrow, const int ncol, const int l, double *v,
                    std::ofstream &outfile)
{
  // Write P_l\P_{l-1}

  int stride = std::pow (2, l);
  int Cstride = stride * 2;

  int row_counter = 0;

  for (int irow = 0; irow < nrow; irow += stride)
    {
      if (row_counter % 2 == 0)
        {
          for (int jcol = Cstride; jcol < ncol; jcol += Cstride)
            {
              outfile.write (reinterpret_cast<char *> (
                                 &v[get_index (ncol, irow, jcol - stride)]),
                             sizeof (double));
            }
        }
      else
        {
          for (int jcol = 0; jcol < ncol; jcol += stride)
            {
              outfile.write (
                  reinterpret_cast<char *> (&v[get_index (ncol, irow, jcol)]),
                  sizeof (double));
            }
        }
      ++row_counter;
    }
}

void
pi_lminus1 (const int l, std::vector<double> &v0)
{
  int nlevel = static_cast<int> (std::log2 (v0.size () - 1));
  int my_level = nlevel - l;
  int stride = std::pow (2, l); // current stride
  //  int Pstride = stride/2; //finer stride
  int Cstride = stride * 2; // coarser stride

  if (my_level != 0)
    {
      for (auto it0 = v0.begin () + Cstride; it0 < v0.end (); it0 += Cstride)
        {
          *(it0 - stride) -= 0.5 * (*it0 + *(it0 - Cstride));
        }
    }
}

void
pi_Ql (const int nrow, const int ncol, const int l, double *v,
       std::vector<double> &row_vec, std::vector<double> &col_vec)
{
  // Restrict data to coarser level

  int stride = std::pow (2, l); // current stride
  //  int Pstride = stride/2; //finer stride
  int Cstride = stride * 2; // coarser stride

  //  std::vector<double> row_vec(ncol), col_vec(nrow)   ;

  for (int irow = 0; irow < nrow;
       irow += Cstride) // Do the rows existing  in the coarser level
    {
      for (int jcol = 0; jcol < ncol; ++jcol)
        {
          row_vec[jcol] = v[get_index (ncol, irow, jcol)];
        }

      pi_lminus1 (l, row_vec);

      for (int jcol = 0; jcol < ncol; ++jcol)
        {
          v[get_index (ncol, irow, jcol)] = row_vec[jcol];
        }
    }

  if (nrow > 1)
    {
      for (int jcol = 0; jcol < ncol;
           jcol += Cstride) // Do the columns existing  in the coarser level
        {
          for (int irow = 0; irow < nrow; ++irow)
            {
              col_vec[irow] = v[get_index (ncol, irow, jcol)];
            }

          pi_lminus1 (l, col_vec);

          for (int irow = 0; irow < nrow; ++irow)
            {
              v[get_index (ncol, irow, jcol)] = col_vec[irow];
            }
        }

      // Now the new-new stuff

      for (int irow = Cstride; irow <= nrow - 1 - Cstride; irow += 2 * Cstride)
        {
          for (int jcol = Cstride; jcol <= ncol - 1 - Cstride;
               jcol += 2 * Cstride)
            {
              v[get_index (ncol, irow - stride, jcol - stride)]
                  -= 0.25
                     * (v[get_index (ncol, irow - Cstride, jcol - Cstride)]
                        + v[get_index (ncol, irow - Cstride, jcol)]
                        + v[get_index (ncol, irow, jcol)]
                        + v[get_index (ncol, irow, jcol - Cstride)]);

              v[get_index (ncol, irow - stride, jcol + stride)]
                  -= 0.25
                     * (v[get_index (ncol, irow - Cstride, jcol)]
                        + v[get_index (ncol, irow - Cstride, jcol + Cstride)]
                        + v[get_index (ncol, irow, jcol + Cstride)]
                        + v[get_index (ncol, irow, jcol)]);

              v[get_index (ncol, irow + stride, jcol + stride)]
                  -= 0.25
                     * (v[get_index (ncol, irow, jcol)]
                        + v[get_index (ncol, irow, jcol + Cstride)]
                        + v[get_index (ncol, irow + Cstride, jcol + Cstride)]
                        + v[get_index (ncol, irow + Cstride, jcol)]);

              v[get_index (ncol, irow + stride, jcol - stride)]
                  -= 0.25
                     * (v[get_index (ncol, irow, jcol - Cstride)]
                        + v[get_index (ncol, irow, jcol)]
                        + v[get_index (ncol, irow + Cstride, jcol)]
                        + v[get_index (ncol, irow + Cstride, jcol - Cstride)]);
            }
        }
    }
}

void
assign_num_level (const int nrow, const int ncol, const int l, double *v,
                  double num)
{
  // set the value of nodal values at level l to number num

  int stride = std::pow (2, l); // current stride

  for (int irow = 0; irow < nrow; irow += stride)
    {
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          v[get_index (ncol, irow, jcol)] = num;
        }
    }
}

void
copy_level (const int nrow, const int ncol, const int l, double *v,
            std::vector<double> &work)
{

  int stride = std::pow (2, l); // current stride

  for (int irow = 0; irow < nrow; irow += stride)
    {
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          work[get_index (ncol, irow, jcol)] = v[get_index (ncol, irow, jcol)];
        }
    }
}

void
add_level (const int nrow, const int ncol, const int l, double *v,
           double *work)
{
  // v += work at level l

  int stride = std::pow (2, l); // current stride

  for (int irow = 0; irow < nrow; irow += stride)
    {
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          v[get_index (ncol, irow, jcol)]
              += work[get_index (ncol, irow, jcol)];
        }
    }
}

void
subtract_level (const int nrow, const int ncol, const int l, double *v,
                double *work)
{
  // v += work at level l
  int stride = std::pow (2, l); // current stride

  for (int irow = 0; irow < nrow; irow += stride)
    {
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          v[get_index (ncol, irow, jcol)]
              -= work[get_index (ncol, irow, jcol)];
        }
    }
}

void
compute_correction_loadv (const int l, std::vector<double> &v)
{
  int stride = std::pow (2, l); // current stride
  int Pstride = stride / 2;     // finer stride

  auto it = v.begin () + Pstride;
  v.front () += 0.25 * (*it); // first element
  for (auto it = std::begin (v) + stride; it <= std::end (v) - stride;
       it += stride)
    {
      *(it) += 0.25 * (*(it - Pstride) + *(it + Pstride));
    }
  it = v.end () - Pstride - 1;
  v.back () += 0.25 * (*it); // last element
}

void
qwrite_level_2D (const int nrow, const int ncol, const int nlevel, const int l,
                 double *v, double tol, const std::string outfile)
{

  int stride = std::pow (2, l);

  double norm = 0;

  for (int irow = 0; irow < nrow; irow += stride)
    {
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          double ntest = std::abs (v[get_index (ncol, irow, jcol)]);
          if (ntest > norm)
            norm = ntest;
        }
    }

  tol /= (double)(nlevel + 1);
  double coeff = norm * tol;

  gzFile out_file = gzopen (outfile.c_str (), "w9b");
  gzwrite (out_file, &coeff, sizeof (double));

  int prune_count = 0;

  for (int l = 0; l <= nlevel; l++)
    {
      int stride = std::pow (2, l);
      int Cstride = stride * 2;
      int row_counter = 0;

      for (int irow = 0; irow < nrow; irow += stride)
        {
          if (row_counter % 2 == 0 && l != nlevel)
            {
              for (int jcol = Cstride; jcol < ncol; jcol += Cstride)
                {
                  int quantum = (int)(v[get_index (ncol, irow, jcol - stride)]
                                      / coeff);
                  if (quantum == 0)
                    ++prune_count;
                  gzwrite (out_file, &quantum, sizeof (int));
                }
            }
          else
            {
              for (int jcol = 0; jcol < ncol; jcol += stride)
                {
                  int quantum = (int)(v[get_index (ncol, irow, jcol)] / coeff);
                  if (quantum == 0)
                    ++prune_count;
                  gzwrite (out_file, &quantum, sizeof (int));
                }
            }
          ++row_counter;
        }
    }

  std::cout << "Pruned : " << prune_count << " Reduction : "
            << (double)nrow * ncol / (nrow * ncol - prune_count) << "\n";
  gzclose (out_file);
}

void
quantize_2D_iterleave (const int nrow, const int ncol, double *v,
                       std::vector<int> &work, double norm, double tol)
{
  //  std::cout << "Tolerance: " << tol << "\n";
  int size_ratio = sizeof (double) / sizeof (int);


  //std::cout << "Norm of sorts: " << norm << "\n";

  //    double quantizer = 2.0*norm * tol;
    double quantizer = 2.0*norm * tol;
  //std::cout << "Quantization factor: " << quantizer << "\n";
  std::memcpy (work.data (), &quantizer, sizeof (double));

  int prune_count = 0;

  for (int index = 0; index < ncol * nrow; ++index)
    {
      int quantum = (int)(v[index] / quantizer );
      work[index + size_ratio] = quantum;
      if (quantum == 0)
        ++prune_count;
    }

  //std::cout << "Pruned : " << prune_count << " Reduction : "
  //          << (double)2 * nrow * ncol / (nrow * ncol - prune_count) << "\n";
}

void
dequantize_2D_iterleave (const int nrow, const int ncol, double *v,
                         const std::vector<int> &work)
{
  int size_ratio = sizeof (double) / sizeof (int);
  double quantizer;

  std::memcpy (&quantizer, work.data(), sizeof (double));

  for (int index = 0; index < nrow * ncol; ++index)
    {
      v[index] = quantizer * double(work[index + size_ratio]);
    }
}

void
zwrite_2D_interleave (std::vector<int> &qv, const std::string outfile)
{
  gzFile out_file = gzopen (outfile.c_str (), "w6b");
  gzwrite (out_file, qv.data (), qv.size () * sizeof (int));
  gzclose (out_file);
}

void
qwrite_2D_interleave (const int nrow, const int ncol, const int nlevel,
                      const int l, double *v, double tol,
                      const std::string outfile)
{

  int stride = std::pow (2, l); // current stride

  double norm = 0;

  for (int irow = 0; irow < nrow; irow += stride)
    {
      for (int jcol = 0; jcol < ncol; jcol += stride)
        {
          double ntest = std::abs (v[get_index (ncol, irow, jcol)]);
          if (ntest > norm)
            norm = ntest;
        }
    }

  tol /= (double)(nlevel + 1);

  double coeff = norm * tol;
  std::cout << "Quantization factor: " << coeff << "\n";

  gzFile out_file = gzopen (outfile.c_str (), "w6b");
  int prune_count = 0;
  gzwrite (out_file, &coeff, sizeof (double));

  for (auto index = 0; index < ncol * nrow; ++index)
    {
      int quantum = (int)(v[index] / coeff);
      if (quantum == 0)
        ++prune_count;
      gzwrite (out_file, &quantum, sizeof (int));
    }

  std::cout << "Pruned : " << prune_count << " Reduction : "
            << (double)nrow * ncol / (nrow * ncol - prune_count) << "\n";
  gzclose (out_file);
}

void
compress_memory (void *in_data, size_t in_data_size,
                 std::vector<uint8_t> &out_data)
{
  std::vector<uint8_t> buffer;

  const size_t BUFSIZE = 128 * 1024;
  uint8_t temp_buffer[BUFSIZE];

  z_stream strm;
  strm.zalloc = 0;
  strm.zfree = 0;
  strm.next_in = reinterpret_cast<uint8_t *> (in_data);
  strm.avail_in = in_data_size;
  strm.next_out = temp_buffer;
  strm.avail_out = BUFSIZE;

  deflateInit (&strm, Z_BEST_COMPRESSION);

  while (strm.avail_in != 0)
    {
      int res = deflate (&strm, Z_NO_FLUSH);
      assert (res == Z_OK);
      if (strm.avail_out == 0)
        {
          buffer.insert (buffer.end (), temp_buffer, temp_buffer + BUFSIZE);
          strm.next_out = temp_buffer;
          strm.avail_out = BUFSIZE;
        }
    }

  int deflate_res = Z_OK;
  while (deflate_res == Z_OK)
    {
      if (strm.avail_out == 0)
        {
          buffer.insert (buffer.end (), temp_buffer, temp_buffer + BUFSIZE);
          strm.next_out = temp_buffer;
          strm.avail_out = BUFSIZE;
        }
      deflate_res = deflate (&strm, Z_FINISH);
    }

  assert (deflate_res == Z_STREAM_END);
  buffer.insert (buffer.end (), temp_buffer,
                 temp_buffer + BUFSIZE - strm.avail_out);
  deflateEnd (&strm);

  out_data.swap (buffer);
}

void
decompress_memory (const void *src, int srcLen, void *dst, int dstLen)
{
  z_stream strm = { 0 };
  strm.total_in = strm.avail_in = srcLen;
  strm.total_out = strm.avail_out = dstLen;
  strm.next_in = (Bytef *)src;
  strm.next_out = (Bytef *)dst;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  int err = -1;
  int ret = -1;

  err = inflateInit2 (&strm, (15 + 32)); // 15 window bits, and the +32 tells
                                         // zlib to to detect if using gzip or
                                         // zlib
  if (err == Z_OK)
    {
      err = inflate (&strm, Z_FINISH);
      if (err == Z_STREAM_END)
        {
          ret = strm.total_out;
        }
      else
        {
          inflateEnd (&strm);
          //             return err;
        }
    }
  else
    {
      inflateEnd (&strm);
      //        return err;
    }

  inflateEnd (&strm);
  //    return ret;
}

void
qread_level_2D (const int nrow, const int ncol, const int nlevel, double *v,
                std::string infile)
{
  int buff_size = 128 * 1024;
  unsigned char unzip_buffer[buff_size];
  int int_buffer[buff_size / sizeof (int)];
  unsigned int unzipped_bytes, total_bytes = 0;
  double coeff;

  gzFile in_file_z = gzopen (infile.c_str (), "r");
  std::cout << in_file_z << "\n";

  unzipped_bytes = gzread (in_file_z, unzip_buffer,
                           sizeof (double)); // read the quantization constant
  std::memcpy (&coeff, &unzip_buffer, unzipped_bytes);

  int last = 0;
  while (true)
    {
      unzipped_bytes = gzread (in_file_z, unzip_buffer, buff_size);
      std::cout << unzipped_bytes << "\n";
      if (unzipped_bytes > 0)
        {
          total_bytes += unzipped_bytes;
          int num_int = unzipped_bytes / sizeof (int);

          std::memcpy (&int_buffer, &unzip_buffer, unzipped_bytes);
          for (int i = 0; i < num_int; ++i)
            {
              v[last] = double(int_buffer[i]) * coeff;
              ++last;
            }
        }
      else
        {
          break;
        }
    }

  gzclose (in_file_z);
}

//       unzippedBytes = gzread(inFileZ, unzipBuffer, buff_size);
//       std::cout << "Read: "<< unzippedBytes <<"\n";
//       std::memcpy(&v[irow][0], &unzipBuffer, unzippedBytes);
//     }

//   gzclose(inFileZ);
// }

void
set_number_of_levels (const int nrow, const int ncol, int &nlevel)
{
  // set the depth of levels in isotropic case
  if (nrow == 1)
    {
      nlevel = static_cast<int> (std::log2 (ncol - 1));
    }
  else if (nrow > 1)
    {
      int nlevel_x = static_cast<int> (std::log2 (nrow - 1));
      int nlevel_y = static_cast<int> (std::log2 (ncol - 1));
      nlevel = std::min (nlevel_x, nlevel_y);
    }
}

void
refactor (const int nrow, const int ncol, const int l_target, double *v,
          std::vector<double> &work, std::vector<double> &row_vec,
          std::vector<double> &col_vec)
{
  // refactor
  //  std::cout << "refactoring" << "\n";

  for (int l = 0; l < l_target; ++l)
    {

      int stride = std::pow (2, l); // current stride
      int Cstride = stride * 2;     // coarser stride

      pi_Ql (nrow, ncol, l, v, row_vec,
             col_vec); // rename!. v@l has I-\Pi_l Q_l+1 u
      copy_level (nrow, ncol, l, v,
                  work); // copy the nodal values of v on l  to matrix work

      assign_num_level (nrow, ncol, l + 1, work.data (), 0.0);

      // row-sweep
      for (int irow = 0; irow < nrow; ++irow)
        {
          for (int jcol = 0; jcol < ncol; ++jcol)
            {
              row_vec[jcol] = work[get_index (ncol, irow, jcol)];
            }

          mass_matrix_multiply (l, row_vec);

          restrict (l + 1, row_vec);

          solve_tridiag_M (l + 1, row_vec);

          for (int jcol = 0; jcol < ncol; ++jcol)
            {
              work[get_index (ncol, irow, jcol)] = row_vec[jcol];
            }
        }


      // column-sweep
      if (nrow > 1) // do this if we have an 2-dimensional array
        {
          for (int jcol = 0; jcol < ncol; jcol += Cstride)
            {
              for (int irow = 0; irow < nrow; ++irow)
                {
                  col_vec[irow] = work[get_index (ncol, irow, jcol)];
                }

              mass_matrix_multiply (l, col_vec);

              restrict (l + 1, col_vec);
              solve_tridiag_M (l + 1, col_vec);

              for (int irow = 0; irow < nrow; ++irow)
                {
                  work[get_index (ncol, irow, jcol)] = col_vec[irow];
                }
            }
        }

      // Solved for (z_l, phi_l) = (c_{l+1}, vl)

      add_level (nrow, ncol, l + 1, v,
                 work.data ()); // Qu_l = \Pi_l Q_{l+1}u + z_l
    }


}

void
recompose (const int nrow, const int ncol, const int l_target, double *v,
           std::vector<double> &work, std::vector<double> &row_vec,
           std::vector<double> &col_vec)
{

  // recompose

  for (int l = l_target; l > 0; --l)
    {

      int stride = std::pow (2, l); // current stride
      int Pstride = stride / 2;

      copy_level (nrow, ncol, l - 1, v, work); // copy the nodal values of cl
                                               // on l-1 (finer level)  to
                                               // matrix work
      assign_num_level (nrow, ncol, l, work.data (),
                        0.0); // zero out nodes of l on cl

      // row-sweep
      for (int irow = 0; irow < nrow; ++irow)
        {
          for (int jcol = 0; jcol < ncol; ++jcol)
            {
              row_vec[jcol] = work[get_index (ncol, irow, jcol)];
            }

          mass_matrix_multiply (l - 1, row_vec);

          restrict (l, row_vec);
          solve_tridiag_M (l, row_vec);

          for (int jcol = 0; jcol < ncol; ++jcol)
            {
              work[get_index (ncol, irow, jcol)] = row_vec[jcol];
            }
        }

      // column-sweep, this is the slow one! Need something like column_copy
      if (nrow > 1) // check if we have 1-D array..
        {
          for (int jcol = 0; jcol < ncol; jcol += stride)
            {
              for (int irow = 0; irow < nrow; ++irow)
                {
                  col_vec[irow] = work[get_index (ncol, irow, jcol)];
                }

              mass_matrix_multiply (l - 1, col_vec);

              restrict (l, col_vec);
              solve_tridiag_M (l, col_vec);

              for (int irow = 0; irow < nrow; ++irow)
                {
                  work[get_index (ncol, irow, jcol)] = col_vec[irow];
                }
            }
        }
      subtract_level (nrow, ncol, l, work.data (), v); // do -(Qu - zl)


      // row-sweep
      for (int irow = 0; irow < nrow; irow += stride)
        {
          for (int jcol = 0; jcol < ncol; ++jcol)
            {
              row_vec[jcol] = work[get_index (ncol, irow, jcol)];
            }

          interpolate_from_level_nMl (l, row_vec);

          for (int jcol = 0; jcol < ncol; ++jcol)
            {
              work[get_index (ncol, irow, jcol)] = row_vec[jcol];
            }
        }

      // column-sweep, this is the slow one! Need something like column_copy
      if (nrow > 1)
        {
          for (int jcol = 0; jcol < ncol; jcol += Pstride)
            {
              for (int irow = 0; irow < nrow; ++irow) // copy all rows
                {
                  col_vec[irow] = work[get_index (ncol, irow, jcol)];
                }

              interpolate_from_level_nMl (l, col_vec);

              for (int irow = 0; irow < nrow; ++irow)
                {
                  work[get_index (ncol, irow, jcol)] = col_vec[irow];
                }
            }
        }
      assign_num_level (nrow, ncol, l, v, 0.0); // zero out nodes of l on cl
      subtract_level (nrow, ncol, l - 1, v, work.data ());
    }
}

inline double
interp_2d (double q11, double q12, double q21, double q22, double x1,
           double x2, double y1, double y2, double x, double y)
{
  double x2x1, y2y1, x2x, y2y, yy1, xx1;
  x2x1 = x2 - x1;
  y2y1 = y2 - y1;
  x2x = x2 - x;
  y2y = y2 - y;
  yy1 = y - y1;
  xx1 = x - x1;
  return 1.0 / (x2x1 * y2y1) * (q11 * x2x * y2y + q21 * xx1 * y2y
                                + q12 * x2x * yy1 + q22 * xx1 * yy1);
}

inline double
interp_0d (const double x1, const double x2, const double y1, const double y2,
           const double x)
{
  // do a linear interpolation between (x1, y1) and (x2, y2)
  return (((x2 - x) * y1 + (x - x1) * y2) / (x2 - x1));
}

void
resample_1d (const double *inbuf, double *outbuf, const int ncol,
             const int ncol_new)
{
  double hx_o = 1.0 / double(ncol - 1);
  double hx = 1.0 / double(ncol_new - 1); // x-spacing
  double hx_ratio = (hx_o / hx);          // ratio of x-spacing resampled/orig

  for (int icol = 0; icol < ncol_new - 1; ++icol)
    {
      int i_left = floor (icol / hx_ratio);
      int i_right = i_left + 1;

      double x1 = double(i_left) * hx_o;
      double x2 = double(i_right) * hx_o;

      double y1 = inbuf[i_left];
      double y2 = inbuf[i_right];
      double x = double(icol) * hx;
      //      std::cout <<  x1 << "\t" << x2 << "\t" << x << "\t"<< "\n";
      // std::cout <<  y1 << "\t" << y2 << "\t" << "\n";

      outbuf[icol] = interp_0d (x1, x2, y1, y2, x);
      //      std:: cout << mgard_interp_0d( x1,  x2,  y1,  y2,  x) << "\n";
    }

  outbuf[ncol_new - 1] = inbuf[ncol - 1];
}


void
resample_1d_inv2 (const double *inbuf, double *outbuf, const int ncol,
             const int ncol_new)
{
  double hx_o = 1.0 / double(ncol - 1);
  double hx = 1.0 / double(ncol_new - 1); // x-spacing
  double hx_ratio = (hx_o / hx);          // ratio of x-spacing resampled/orig

  for (int icol = 0; icol < ncol_new - 1; ++icol)
    {
      int i_left = floor (icol / hx_ratio);
      int i_right = i_left + 1;

      double x1 = double(i_left) * hx_o;
      double x2 = double(i_right) * hx_o;

      double y1 = inbuf[i_left];
      double y2 = inbuf[i_right];
      double x = double(icol) * hx;

      double d1 = std::pow(x1 - x, 4.0 );
      double d2 = std::pow(x2 - x, 4.0 );
      
      
      if( d1 == 0)
        {
          outbuf[icol] = y1;
        }
      else if( d2 == 0)
        {
          outbuf[icol] = y2;
        }
      else{
        double dsum = 1.0/d1 + 1.0/d2;
        outbuf[icol] =  (y1/d1 + y2/d2)/dsum;
      }
    }

  outbuf[ncol_new - 1] = inbuf[ncol - 1];
}

void
resample_2d (const double *inbuf, double *outbuf, const int nrow,
             const int ncol, const int nrow_new, const int ncol_new)
{
  double hx_o = 1.0 / double(ncol - 1);
  double hx = 1.0 / double(ncol_new - 1); // x-spacing
  double hx_ratio = (hx_o / hx);          // ratio of x-spacing resampled/orig

  double hy_o = 1.0 / double(nrow - 1);
  double hy = 1.0 / double(nrow_new - 1); // x-spacing
  double hy_ratio = (hy_o / hy);          // ratio of x-spacing resampled/orig

  for (int irow = 0; irow < nrow_new - 1; ++irow)
    {
      int i_bot = floor (irow / hy_ratio);
      int i_top = i_bot + 1;

      double y = double(irow) * hy;
      double y1 = double(i_bot) * hy_o;
      double y2 = double(i_top) * hy_o;

      for (int jcol = 0; jcol < ncol_new - 1; ++jcol)
        {
          int j_left = floor (jcol / hx_ratio);
          int j_right = j_left + 1;

          double x = double(jcol) * hx;
          double x1 = double(j_left) * hx_o;
          double x2 = double(j_right) * hx_o;

          double q11 = inbuf[get_index (ncol, i_bot, j_left)];
          double q12 = inbuf[get_index (ncol, i_top, j_left)];
          double q21 = inbuf[get_index (ncol, i_bot, j_right)];
          double q22 = inbuf[get_index (ncol, i_top, j_right)];

          outbuf[get_index (ncol_new, irow, jcol)]
              = interp_2d (q11, q12, q21, q22, x1, x2, y1, y2, x, y);
        }

      // last column
      double q1 = inbuf[get_index (ncol, i_bot, ncol - 1)];
      double q2 = inbuf[get_index (ncol, i_top, ncol - 1)];
      outbuf[get_index (ncol_new, irow, ncol_new - 1)]
          = interp_0d (y1, y2, q1, q2, y);
    }

  // last-row
  resample_1d (&inbuf[get_index (ncol, nrow - 1, 0)],
               &outbuf[get_index (ncol_new, nrow_new - 1, 0)], ncol, ncol_new);
}





void
resample_2d_inv2 (const double *inbuf, double *outbuf, const int nrow,
             const int ncol, const int nrow_new, const int ncol_new)
{
  double hx_o = 1.0 / double(ncol - 1);
  double hx = 1.0 / double(ncol_new - 1); // x-spacing
  double hx_ratio = (hx_o / hx);          // ratio of x-spacing resampled/orig

  double hy_o = 1.0 / double(nrow - 1);
  double hy = 1.0 / double(nrow_new - 1); // x-spacing
  double hy_ratio = (hy_o / hy);          // ratio of x-spacing resampled/orig

  for (int irow = 0; irow < nrow_new - 1; ++irow)
    {
      int i_bot = floor (irow / hy_ratio);
      int i_top = i_bot + 1;

      double y = double(irow) * hy;
      double y1 = double(i_bot) * hy_o;
      double y2 = double(i_top) * hy_o;

      for (int jcol = 0; jcol < ncol_new - 1; ++jcol)
        {
          int j_left = floor (jcol / hx_ratio);
          int j_right = j_left + 1;

          double x = double(jcol) * hx;
          double x1 = double(j_left) * hx_o;
          double x2 = double(j_right) * hx_o;

          double q11 = inbuf[get_index (ncol, i_bot, j_left)];
          double q12 = inbuf[get_index (ncol, i_top, j_left)];
          double q21 = inbuf[get_index (ncol, i_bot, j_right)];
          double q22 = inbuf[get_index (ncol, i_top, j_right)];

          double d11 = (std::pow(x1 - x, 2.0 ) +  std::pow(y1 - y, 2.0 ));
          double d12 = (std::pow(x1 - x, 2.0 ) +  std::pow(y2 - y, 2.0 ));
          double d21 = (std::pow(x2 - x, 2.0 ) +  std::pow(y1 - y, 2.0 ));
          double d22 = (std::pow(x2 - x, 2.0 ) +  std::pow(y2 - y, 2.0 ));

          
          if( d11 == 0 )
            {
              outbuf[get_index (ncol_new, irow, jcol)] = q11;
            }
          else if (d12 == 0)
            {
              outbuf[get_index (ncol_new, irow, jcol)] = q12;
            }
          else if (d21 == 0)
            {
              outbuf[get_index (ncol_new, irow, jcol)] = q21;
            }
          else if (d22 == 0)
            {
              outbuf[get_index (ncol_new, irow, jcol)] = q22;
            }
          else
            {
              d11 = std::pow(d11, 1.5);
              d12 = std::pow(d12, 1.5);
              d21 = std::pow(d21, 1.5);         
              d22 = std::pow(d22, 1.5);         
              
              double dsum = 1.0/(d11) + 1.0/(d12) + 1.0/(d21) + 1.0/(d22);
              //std::cout <<  (q11/d11 + q12/d12 + q21/d21 + q22/d22)/dsum << "\n";
              //              std::cout <<  dsum << "\n";
          
              outbuf[get_index (ncol_new, irow, jcol)] = (q11/d11 + q12/d12 + q21/d21 + q22/d22)/dsum;
            }

        }

      // last column
      double q1 = inbuf[get_index (ncol, i_bot, ncol - 1)];
      double q2 = inbuf[get_index (ncol, i_top, ncol - 1)];

      double d1 = std::pow(y1 - y, 4.0 );
      double d2 = std::pow(y2 - y, 4.0 );

      if( d1 == 0)
        {
          outbuf[get_index (ncol_new, irow, ncol_new - 1)] = q1;
        }
      else if( d2 == 0)
        {
          outbuf[get_index (ncol_new, irow, ncol_new - 1)] = q2;
        }
      else{
        double dsum = 1.0/d1 + 1.0/d2;
        outbuf[get_index (ncol_new, irow, ncol_new - 1)] = (q1/d1 + q2/d2)/dsum;
                   
      }

    }

  // last-row
  resample_1d_inv2 (&inbuf[get_index (ncol, nrow - 1, 0)],
               &outbuf[get_index (ncol_new, nrow_new - 1, 0)], ncol, ncol_new);
}


} //end namespace mgard
