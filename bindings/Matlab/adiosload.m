function adiosload(file, prefix)
%ADIOSLOAD Read all variables in an ADIOS BP file
%
%   ADIOSLOAD is a batch process to load all variables in an ADIOS BP 
%   file. The simplest way is to give an ADIOS BP file name. An optional 
%   'prefix' string can be given to avoid name conflict or name conversion. 
%
%   ADIOSLOAD(FILE)
%      Read all the variables in FILE and use the same variable names.
%
%   ADIOSLOAD(FILE, PREFIX)
%      Read all the variables in FILE and add PREFIX to the varaible names.
%
%   See also ADIOSOPEN, ADIOSCLOSE, ADIOS.

%   Copyright 2009 UT-BATTELLE, LLC
%   $Revision: 1.0 $  $Date: 2009/08/05 12:53:41 $
%   Author: Norbert Podhorszki <pnorbert@ornl.gov>

if (~exist('prefix', 'var'))
    prefix = '';
end
fp = adiosopen(file);
for i = 1:length(fp.Groups.Variables)
    try
        name{i} = fp.Groups.Variables(i).Name;
        data{i} = adiosread(fp.Groups, fp.Groups.Variables(i).Name);
        assignin('base',[prefix name{i}],data{i});
    catch
        warning(['Skip ... ', fp.Groups.Variables(i).Name]);
    end
end
adiosclose(fp);
