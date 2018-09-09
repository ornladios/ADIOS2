function [data, attributes] = adiosread(varargin)
%ADIOSREAD Read data from an ADIOS BP file.
%   
%   ADIOSREAD can read in data with different call options.
%   The simplest way is to give the name of the file and the path
%   to a variable/attribute. If you have more than one adios groups
%   stored in one file, you should supply the group name as well.
%   In these cases, the library opens the file, reads in the data
%   and closes the file. 
%
%   Note: the adios group is not the same as HDF5 groups. Each variable has
%   a path, which defines a logical hierarchy of the variables within one 
%   adios group. This logical hierarchy is what is similar to HDF5 groups.
%
%   DATA = ADIOSREAD(FILE, VARPATH) 
%      Read complete DATA from FILE with path VARPATH.
%      Use only if you have only one adios group in the file or you know that
%      the VARPATH is in the first adios group in the file.
%
%   DATA = ADIOSREAD(FILE, GROUP, VARPATH) 
%      Read complete DATA from an adios GROUP of FILE with path VARPATH.
%      Use this form if you have more than one ADIOS group in the file.
%      GROUP is either a group name (string) or an index of the groups
%      (integer, starting from 1).
% 
%   DATA = ADIOSREAD(GROUPSTRUCT, VARPATH) 
%      If you have opened the file with ADIOSOPEN, you can supply the
%      the group struct to read in a variable/attribute.
%      GROUPSTRUCT must be one of the info.Groups array.
%      When finished reading all data, close the file with ADIOSCLOSE. 
%
%   DATA... = ADIOSREAD( ..., 'Slice', SLICEDEF)
%      You can read a portion of a variable. 
%      A slice is defined as an N-by-2 array of integers, where N is the 
%      number of dimensions of the variable (or less). A tuple describes the 
%      "start" and "count" values. The "start" values start from 1.
%          E.g. [1 10; 5 2] reads the first 10 values in the first dimension
%      and 2 values from the 5th value in the second dimension resulting in
%      a 10-by-2 array. 
%          You can use negative numbers to index from the end of the array
%      as in python. -1 refers to the last element of the array, -2 the one
%      before and so on. 
%          E.g. [-1 1] reads in the last value of a 1D array. 
%               [1 -1] reads in the complete dimension.
%      If the slice definition has less rows than the number of dimensions
%      of the variable, [1 -1] rows are added automatically to read those
%      dimensions completely.
%          If the slice definition has more rows than the number of dimensions
%      of the variable, the extra slice definitions will be ignored.
%
%   DATA... = ADIOSREAD( ..., 'Verbose', VALUE)
%      To get logging from the adiosread code, set Verbose to 1 or higher.
%      Higher values cause more and more details to be printed. 
%
%   Please read the file adioscopyright.txt for more information.
%
%   See also ADIOSOPEN, ADIOSCLOSE, ADIOS.

%   Copyright 2009 UT-BATTELLE, LLC
%   $Revision: 1.0 $  $Date: 2009/08/05 12:53:41 $
%   Author: Norbert Podhorszki <pnorbert@ornl.gov>

%
% Process arguments.
%

checkArgCounts(varargin{:});
[args, msg] = parse_inputs(varargin{:});
if (~isempty(msg))
    error('MATLAB:adiosread:inputParsing', '%s', msg);
end

if (isnumeric(args.Group))
    if (isa(args.Group, 'int64'))
        gn=sprintf('Group handler=%lld',args.Group);
    else
        gn=sprintf('Group index=%d',args.Group);
    end
else
    gn=sprintf('Group name=%s',args.Group);
end
offsets=sprintf('%d ', args.Offsets);
counts=sprintf('%d ', args.Counts);
verbose=sprintf('%d ', args.Verbose);

input = sprintf('adiosreadc:\n  File name=%s \n  %s \n  Var=%s\n  Offsets=[%s]  Counts=[%s] \n  Verbose=%s', ...
 args.File, gn, args.Path, offsets, counts, verbose);
if (args.Verbose > 0) 
    CallArguments = input
end


if (nargout == 1)
    data = adiosreadc(args.File, args.Group, args.Path, args.Offsets, args.Counts, args.Verbose);
else 
    adiosreadc(args.File, args.Group, args.Path, args.Offsets, args.Counts, args.Verbose);
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

args.File    = '';
args.Group   = '';
args.Path    = '';
args.Offsets = []; % start positions in each dimension for slicing
args.Counts  = []; % counts in each dimension for slicing
args.Verbose = 0;  % verbosity, default is off, i.e. 0

msg = '';

% Arg 1: file name or int64 group handler
if ischar(varargin{1})
    args.File = varargin{1};
    %
    % Verify existence of filename
    %
    fid = fopen(args.File);    
    if (fid == -1)
        % Look for filename with extensions.
        fid = fopen([args.File '.bp']);
    end

    if (fid == -1)
        error('MATLAB:hdf5read:fileOpen', ...
              'Couldn''t open file (%s).', ...
              args.File)
    else
        % Get full filename 
        args.File = fopen(fid);
        fclose(fid);
    end
elseif (isa(varargin{1}, 'struct'))
    try
        grp = varargin{1};
        args.Group = grp.GroupHandler; % int64
    catch
        msg = ['Input is not a group struct of the info struct ' ...
               'returned by ADIOSOPEN'];
        return
    end
else
    msg = ['1st arg should be a string FILE argument to ADIOSREAD ' ...
           'or a group struct from ADIOSOPEN'];
    return
end

% Arg 2 and maybe 3: group and varpath
if (rem(nargin, 2) == 0)
    % even number of arguments: FILE, VARPATH, ...
    args.Path = varargin{2};
    varargin = {varargin{3:end}};
else
    % odd number of arguments: FILE, GROUP, VARPATH, ...
    if (args.Group ~= '') 
        msg = ['GROUPHANDLER, GROUP, VARPATH is not an accepted argument list'];
        return
    end
    args.Group = varargin{2};
    args.Path = varargin{3};
    varargin = {varargin{4:end}};
    % check type of Group
    if ((~ischar(args.Group)) && (~isnumeric(args.Group))) 
        msg = ['GROUP input argument to ADIOSREAD must be a string or a number'];
        return
    end
    if (isnumeric(args.Group))
        % convert group index to int32
        args.Group = int32(args.Group);
    end
end
% check type of Path
if ~ischar(args.Path)
    msg = ['VARPATH input argument to ADIOSREAD must be string.'];
    return
end

% Parse optional arguments based on their number.
if (length(varargin) > 0)
    
    paramStrings = {'slice', 'verbose'};
    
    % For each pair
    for k = 1:2:length(varargin)
        param = lower(varargin{k});
            
        if (~ischar(param))
            msg = 'Parameter name must be a string.';
            return
        end
        
        idx = strmatch(param, paramStrings);
        
        if (isempty(idx))
            msg = sprintf('Unrecognized parameter name "%s".', param);
            return
        elseif (length(idx) > 1)
            msg = sprintf('Ambiguous parameter name "%s".', param);
            return
        end

        switch (paramStrings{idx})
        % SLICE
        case 'slice'
            if (k == length(varargin))
                msg = 'No slicing value specified for Slice option.';
                return
            end
        
            slices = varargin{k+1};
            if ((~isnumeric(slices)) || ...
                (~isempty(slices) && size(slices, 2) ~= 2) || ...
                (~isempty(find(rem(slices, 1) ~= 0))))

                msg = 'Slice values must be n-by-2 array of integers.';
                return
            end

            if (~isempty(slices))
                args.Offsets = int64(fix(slices(:,1)));
                args.Counts = int64(fix(slices(:,2)));
            else
                args.Offsets = int64([]);
                args.Counts = int64([]);
            end
            
        % VERBOSE
        case 'verbose'
            if (k == length(varargin))
                msg = 'No value specified for Verbose option.';
                return
            end
        
            args.Verbose = varargin{k+1};
            if ((~isnumeric(args.Verbose)) || ...
                (~isempty(find(rem(args.Verbose, 1) ~= 0))))
                
                msg = sprintf('''VERBOSE'' must be an integer.');
                return
            end
            if (args.Verbose < 0)
                msg = sprintf('''VERBOSE'' must be greater or equal to zero.');
                return
            end
        end
    end
end
