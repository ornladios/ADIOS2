#include <stdio.h>
#include <string.h>

#include <evpath.h>
#include <fm.h>

#include "dp_interface.h"
#include "sst_data.h"

extern CP_DP_Interface LoadRdmaDP();
extern CP_DP_Interface LoadEVpathDP();

CP_DP_Interface LoadDP(char *dp_name)
{
    if (strcmp(dp_name, "evpath") == 0)
    {
        return LoadEVpathDP();
    }
    else if (strcmp(dp_name, "rdma") == 0)
    {
        return LoadRdmaDP();
    }
    else
    {
        fprintf(stderr, "Unknown DP interface %s, load failed\n", dp_name);
        return NULL;
    }
}
