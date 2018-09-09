function adios()
%ADIOS Reader for the ADIOS BP file format
%   
%   ADIOS is a componentized I/O layer for high performance output combined
%   with an easy to use interface and a set of different data writing methods
%   so that everyone can choose the best method for a particular system.
%
%   ADIOSREAD can be used in itself to read in a variable with a known name.
%   ADIOSOPEN returns a structure with information on all adios groups, 
%   variables and attributes. The structure can be used to read many variables
%   without repeatedly opening and closing the file behind the scenes. 
%   ADIOSCLOSE closes the file, opened by ADIOSOPEN.
%
%   Organization of an ADIOS BP file
%   --------------------------------
%   There is usually one adios group in a file, but there can be more. 
%   An adios group is the set of all variables and attributes that have been
%   written by a (parallel) program at one place in the code. The programmer
%   may have decided to write more than one groups into the file.
%   Note that the adios group is not the same as HDF5 groups. Each variable in
%   the group has a path, which defines a logical hierarchy of the variables 
%   within one adios group. This logical hierarchy is similar to the hierarchy
%   in a HDF5 file.
%
%   Therefore, ADIOSREAD expects one of the info.Groups(idx) structure to
%   specify a group to read from, instead of the info structure itself, where
%   info is returned by ADIOSOPEN.
%
%   Time dimension of a variable
%   ----------------------------
%   Variables can be written several times from a program, if they have a time
%   dimension. The reader exposes the variables with an extra dimension, i.e.
%   a 2D variable written over time is seen as a 3D variable. In MATLAB, the
%   extra dimension is the last dimension (the slowest changing dimension).
%   Since the reader allows reading an arbitrary slice of a variable, data for
%   one timestep can be read in with slicing.
%
%   Extra information provided by ADIOSOPEN
%   ---------------------------------------
%   The ADIOS BP format stores the min/max values in each variable. 
%   The info structure therefore contains these min/max values. There is
%   practically no overhead to provide this information (along with the
%   values of all attributes) even for file sizes of several terabytes.
%
%   Please read the file COPYING in the top directory of the ADIOS source
%   distribution for information on the copyrights.
%
%   See also ADIOSOPEN, ADIOSREAD, ADIOSCLOSE.

%   Copyright 2009 UT-BATTELLE, LLC
%   $Revision: 1.0 $  $Date: 2009/08/05 12:53:41 $
%   Author: Norbert Podhorszki <pnorbert@ornl.gov>

help adios
