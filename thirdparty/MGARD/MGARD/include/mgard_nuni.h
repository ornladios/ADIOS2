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

namespace mgard_common
{

  int  parse_cmdl(int argc, char**argv, int& nrow, int& ncol, double& tol, std::string& in_file, std::string& coord_file);
  


  bool is_2kplus1(double num);
  
  inline int get_index(const int ncol, const int i, const int j);



  double max_norm(const std::vector<double>& v);


  inline double
  interp_2d(double q11, double q12, double q21, double q22, double x1, double x2, double y1, double y2, double x, double y);

  
  inline double get_h(const std::vector<double>& coords, int i, int stride);

  inline double get_dist(const std::vector<double>& coords, int i, int j);

  void qread_2D_interleave( const int nrow, const int ncol, const int nlevel, double* v, std::string infile);


  
  void qwrite_2D_interleave( const int nrow, const int ncol, const int nlevel,  const int  l,   double* v, double tol, double norm, const std::string outfile);
  
  
}


namespace mgard_cannon
{

  void assign_num_level(const int nrow, const int ncol,    const int  l, double* v, double num);

  
  void subtract_level(const int nrow, const int ncol,  const int  l, double* v, double* work);


  void pi_lminus1(const int  l, std::vector<double>& v,  const std::vector<double>& coords);


  void restrict(const int  l, std::vector<double>& v, const std::vector<double>& coords);



  void prolongate(const int  l, std::vector<double>& v, const std::vector<double>& coords);


  void solve_tridiag_M(const int  l, std::vector<double>& v, const std::vector<double>& coords);



  void mass_matrix_multiply(const int  l, std::vector<double>& v, const std::vector<double>& coords);


  void write_level_2D( const int nrow, const int ncol, const int  l,   double* v, std::ofstream& outfile);


  void copy_level(const int nrow, const int ncol,  const int  l, double* v, std::vector<double>& work);

  
}

namespace mgard_gen
{
  inline  double* get_ref(std::vector<double>& v,  const int n, const int no, const int i);
  
  
  inline  int get_lindex(const int n, const int no, const int i);

  
  inline  double get_h_l(const std::vector<double>& coords, const int n, const int no, int i, int stride);


  void write_level_2D_l(const int  l,   double* v, std::ofstream& outfile, int nr, int nc, int nrow, int ncol);

  
  void copy_level_l(const int  l, double* v, double* work, int nr, int nc, int nrow, int ncol);
  

  void subtract_level_l(const int  l, double* v, double* work, int nr, int nc, int nrow, int ncol);

  

  void pi_lminus1_l(const int  l, std::vector<double>& v, const std::vector<double>& coords, int n, int no);


  
  void pi_lminus1_first( std::vector<double>& v,  const std::vector<double>& coords, int n, int no);

  
  void pi_Ql_first(const int nr, const int nc, const int nrow, const int ncol,  const int  l, double* v,  const std::vector<double>& coords_x, const std::vector<double>& coords_y, std::vector<double>& row_vec, std::vector<double>& col_vec);

  void pi_Ql(const int nr, const int nc, const int nrow, const int ncol,  const int  l, double* v,  const std::vector<double>& coords_x, const std::vector<double>& coords_y, std::vector<double>& row_vec, std::vector<double>& col_vec);
  


  void assign_num_level_l(const int  l, double* v, double num, int nr, int nc, const int nrow, const int ncol);
  

  void restrict_first(std::vector<double>& v,  std::vector<double>& coords, int n, int no);


  void solve_tridiag_M_l(const int  l, std::vector<double>& v,  std::vector<double>& coords, int n, int no);



  void add_level_l(const int  l, double* v, double* work, int nr, int nc, int nrow, int ncol);


  void project_first(const int nr, const int nc, const int nrow, const int ncol,  const int l_target, double* v, std::vector<double>& work, std::vector<double>& coords_x, std::vector<double>& coords_y, std::vector<double>& row_vec, std::vector<double>& col_vec );
  
  void prep_2D(const int nr, const int nc, const int nrow, const int ncol,  const int l_target, double* v, std::vector<double>& work, std::vector<double>& coords_x, std::vector<double>& coords_y, std::vector<double>& row_vec, std::vector<double>& col_vec );
  



  void mass_mult_l(const int  l, std::vector<double>& v,  std::vector<double>& coords, const int n, const int no);


  void restrict_l(const int  l, std::vector<double>& v,  std::vector<double>& coords, int n, int no);



  void prolongate_l(const int  l, std::vector<double>& v,  std::vector<double>& coords, int n, int no);

  
  void refactor_2D(const int nr, const int nc, const int nrow, const int ncol,  const int l_target, double* v, std::vector<double>& work, std::vector<double>& coords_x, std::vector<double>& coords_y, std::vector<double>& row_vec, std::vector<double>& col_vec );



  void recompose_2D(const int nr, const int nc, const int nrow, const int ncol,  const int l_target, double* v, std::vector<double>& work, std::vector<double>& coords_x, std::vector<double>& coords_y, std::vector<double>& row_vec, std::vector<double>& col_vec );




  void prolongate_last(std::vector<double>& v,  std::vector<double>& coords, int n, int no);


  void postp_2D(const int nr, const int nc, const int nrow, const int ncol,  const int l_target, double* v, std::vector<double>& work, std::vector<double>& coords_x, std::vector<double>& coords_y, std::vector<double>& row_vec, std::vector<double>& col_vec );


  void qwrite_2D_l(const int nr, const int nc, const int nrow, const int ncol, const int nlevel,  const int  l,   double* v, double tol, double norm, const std::string outfile);
  
}
