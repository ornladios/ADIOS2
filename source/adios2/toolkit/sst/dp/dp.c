#include <stdio.h>
#include <string.h>

#include <evpath.h>
#include <fm.h>

#include "SSTConfig.h"
#include "dp_interface.h"
#include "sst_data.h"

#ifdef SST_HAVE_LIBFABRIC
extern CP_DP_Interface LoadRdmaDP();
#endif /* SST_HAVE_LIBFABRIC */
extern CP_DP_Interface LoadEVpathDP();

CP_DP_Interface LoadDP(char *dp_name)
{
    if (strcmp(dp_name, "evpath") == 0)
    {
        return LoadEVpathDP();
    }
#ifdef SST_HAVE_LIBFABRIC
    else if (strcmp(dp_name, "rdma") == 0)
    {
        return LoadRdmaDP();
    }
#endif /* SST_HAVE_LIBFABRIC */
    else
    {
        fprintf(stderr, "Unknown DP interface %s, load failed\n", dp_name);
        return NULL;
    }
}
