function [data, attributes] = adiosread(varargin)
%ADIOSREAD Read data from an ADIOS BP file.
%   
%   ADIOSREAD reads data from BP file opened with ADIOSOPEN
%   Provide the structure returned by ADIOSOPEN as the first input argument, and the path to a variable.
%   Assume STRUCT = ADIOSOPEN(filepath).
%   Inspect STRUCT.Variables and STRUCT.Attributes for the list of variables 
%   and attributes available in a file.
%
%   DATA = ADIOSREAD(STRUCT, VARPATH) 
%      Read the entire variable VARPATH from a BP file.
%      STRUCT is the output of ADIOSOPEN.
%      VARPATH is a string to a variable or attribute.
%      If an N-dimensional array variable has multiple steps in the file
%      this function reads all steps and returns an N+1 dimensional array
%      where the last dimension equals the number of steps.
%
%   DATA = ADIOSREAD(STRUCT, INDEX) 
%      Read complete DATA from a BP file with path VARPATH.
%      INDEX points to a variable in the STRUCT.Variables() array. 
%
%   Additional parameters:
%
%   DATA = ADIOSREAD(..., START, COUNT, STEPSTART, STEPCOUNT)
%      Read a portion of a variable. 
%
%      START and COUNT:
%      A slice is defined as two arrays of N integers, where N is the 
%      number of dimensions of the variable, describing the
%      "start" and "count" values. The "start" values start from 1.
%          E.g. [1 5], [10 2] reads the first 10 values in the first dimension
%      and 2 values from the 5th position in the second dimension resulting in
%      a 10-by-2 array. 
%          You can use negative numbers to index from the end of the array
%      as in python. -1 refers to the last element of the array, -2 the one
%      before and so on. 
%          E.g. [-1], [1] reads in the last value of a 1D array. 
%               [1], [-1] reads in the complete 1D array.
%
%      STEPSTART and STEPCOUNT:
%      Similarly, the number of steps from a specific step can be read instead
%      of all data. Steps start from 1. Negative index can be used as well.
%          E.g. -1, 1  will read in the last step from the file
%               n, -1  will read all steps from 'n' to the last one
%
%
%
%   Please read the file adioscopyright.txt for more information.
%
%   See also ADIOSOPEN, ADIOSCLOSE, ADIOS.

%   Copyright 2009 UT-BATTELLE, LLC
%   Date: 2018/09/07
%   Author: Norbert Podhorszki <pnorbert@ornl.gov>

%
% Process arguments.
%

checkArgCounts(varargin{:});
[args, msg] = parse_inputs(varargin{:});
if (~isempty(msg))
    error('MATLAB:adiosread:inputParsing', '%s', msg);
end

offsets=sprintf('%d ', args.Starts);
counts=sprintf('%d ', args.Counts);
verbose=sprintf('%d ', args.Verbose);

CallArguments = sprintf('adiosreadc:\n  File name=%s \n  Var=%s\n  Starts=[%s]  Counts=[%s]\n  StepStart=%d  StepCount=%d\n  Verbose=%s', ...
 args.FileName, args.Path, offsets, counts, args.StepStart, args.StepCount, verbose);
if (args.Verbose > 0) 
    CallArguments
end

if (nargout == 1)
    data = adiosreadc(args.File, args.Group, args.Path, args.Starts, args.Counts, args.StepStart, args.StepCount, args.Verbose);
else 
    adiosreadc(args.File, args.Group, args.Path, args.Starts, args.Counts, args.StepStart, args.StepCount, args.Verbose);
end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% FUNCTION:   checkArgCounts %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function checkArgCounts(varargin)

if (nargin < 2)
    error('MATLAB:adiosread:notEnoughInputs', ...
          'ADIOSREAD requires at least two input arguments.')
end


if (nargout > 1)
    error('MATLAB:adiosread:tooManyOutputs', ...
          'ADIOSREAD requires one or fewer output arguments.')
end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% FUNCTION:   parse_inputs   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [args, msg] = parse_inputs(varargin)

args.File    = uint64(0);   % saved file handler
args.FileName = '';         % file name (for debugging purposes)
args.Group   = uint64(0);   % saved IO group handler
args.ADIOS   = uint64(0);   % saved ADIOS handler
args.Path    = '';          % variable path string
args.VarIndex = -1;         % index of variable in struct.Variables()
args.Starts = int64([]);    % start positions in each dimension for slicing
args.Counts  = int64([]);   % counts in each dimension for slicing
args.StepStart = int64(1);  % starting step
args.StepCount = int64(-1); % number of steps to read
args.Verbose = 4;           % verbosity, default is off, i.e. 0

msg = '';

% Arg 1: struct from ADIOSOPEN
if (isa(varargin{1}, 'struct'))
    try
        infostruct = varargin{1};
        args.File = infostruct.Handlers.FileHandler; % uint64
        args.Group = infostruct.Handlers.GroupHandler; % uint64
        args.ADIOS = infostruct.Handlers.ADIOSHandler; % uint64
    catch
        msg = ['1st argument should be the info struct from ADIOSOPEN'];
        return
    end
else
    msg = ['1st argument should be the info struct from ADIOSOPEN'];
    return
end
args.FileName=infostruct.Name;

% Arg 2: varpath or var index
if (ischar(varargin{2}))
    % VARPATH
    args.Path = varargin{2};
    for k = 1:length(infostruct.Variables)
        if( strcmp(f.Variables(k).Name,'T'));
            Args.VarIndex = k;
            break
        end
    end
    if (Args.VarIndex < 0)
        msg = ['2nd argument path does not match any variable names '...
        'in the info struct from ADIOSOPEN'];

    end
elseif (isnumeric(varargin{2}))
    % convert index to int32
    args.VarIndex = int32(varargin{2});
    try 
        % VARPATH
        args.Path = infostruct.Variables(args.VarIndex).Name;
    catch
        msg = ['2nd argument index must be between 1 and number of variables '...
        'in the info struct from ADIOSOPEN'];
        return
    end

else
    msg = ['2nd argument to ADIOSREAD must be a string or a number'];
    return
end

% Arg 3: START array
if (nargin >= 3)
    array = varargin{3};
    if (~isnumeric(array) || isempty(array) || size(array, 1) ~= 1 || ndims(array) ~= 2)
        msg = '3rd argument must be an 1-by-N array of integers.';
        return
    end
    if((isempty(array) || size(array, 1) ~= 1) || ...
       (size(array,2) ~= size(infostruct.Variables(args.VarIndex).Dims,2)))

       msg = sprintf('3rd argument array size must equal to the dimensions of the variable which is %u in case of variable "%s"', args.Path);
       return
    end
    args.Starts = int64(fix(array));
end

% Arg 4: COUNT array
if (nargin >= 4)
    array = varargin{4};
    if (~isnumeric(array) || isempty(array) || size(array, 1) ~= 1 || ndims(array) ~= 2)
        msg = '4th argument must be an 1-by-N array of integers.';
        return
    end
    if((isempty(array) || size(array, 1) ~= 1) || ...
       (size(array,2) ~= size(infostruct.Variables(args.VarIndex).Dims,2)))

       msg = sprintf('4th argument array size must equal to the dimensions of the variable which is %u in case of variable "%s"', args.Path);
       return
    end
    args.Counts = int64(fix(array));
end

% Arg 5: STEPSTART 
if (nargin >= 5)
    value = varargin{5};
    if (~isnumeric(value) || isempty(value) || ndims(value) ~= 2 ||...
       size(value, 1) ~= 1 || size(value, 2) ~= 1 )
        msg = '5th argument must be an 1-by-1 numerical value';
        return
    end
    args.StepStart = int64(fix(value));
end

% Arg 5: STEPCOUNT
if (nargin >= 6)
    value = varargin{6};
    if (~isnumeric(value) || isempty(value) || ndims(value) ~= 2 ||...
       size(value, 1) ~= 1 || size(value, 2) ~= 1 )
        msg = '6th argument must be an 1-by-1 numerical value';
        return
    end
    args.StepCount = int64(fix(value));
end

