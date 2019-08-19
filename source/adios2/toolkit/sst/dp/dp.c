#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <evpath.h>
#include <fm.h>

#include "SSTConfig.h"
#include "dp_interface.h"
#include "sst.h"
#include "sst_data.h"

#ifdef SST_HAVE_LIBFABRIC
extern CP_DP_Interface LoadRdmaDP();
#endif /* SST_HAVE_LIBFABRIC */
#ifdef SST_HAVE_NVSTREAM
extern CP_DP_Interface LoadNvstreamDP();
#endif /* SST_HAVE_LIBFABRIC */
extern CP_DP_Interface LoadEVpathDP();

typedef struct _DPElement
{
    const char *Name;
    CP_DP_Interface Interface;
    long Priority;
} * DPlist;

static DPlist AddDPPossibility(CP_Services Svcs, void *CP_Stream, DPlist List,
                               CP_DP_Interface Interface, const char *Name,
                               struct _SstParams *Params)
{
    int Count = 0;
    if (Interface == NULL)
        return List;
    if (List == NULL)
    {
        List = malloc(2 * sizeof(*List));
    }
    else
    {
        while (List[Count].Interface)
        {
            Count++;
        }
        List = realloc(List, sizeof(*List) * (Count + 2));
    }
    List[Count].Interface = Interface;
    List[Count].Name = Name;
    List[Count].Priority = Interface->getPriority(Svcs, CP_Stream, Params);
    List[Count + 1].Interface = NULL;
    return List;
}

CP_DP_Interface SelectDP(CP_Services Svcs, void *CP_Stream,
                         struct _SstParams *Params)
{
    CP_DP_Interface Ret;
    DPlist List = NULL;
    List = AddDPPossibility(Svcs, CP_Stream, List, LoadEVpathDP(), "evpath",
                            Params);
#ifdef SST_HAVE_LIBFABRIC
    List =
        AddDPPossibility(Svcs, CP_Stream, List, LoadRdmaDP(), "rdma", Params);
#endif /* SST_HAVE_LIBFABRIC */

#ifdef SST_HAVE_NVSTREAM
    List = AddDPPossibility(Svcs, CP_Stream, List, LoadNvstreamDP(), "nvstream",
                            Params);
#endif /* SST_HAVE_LIBFABRIC */

    int SelectedDP = -1;
    int BestPriority = -1;
    int BestPrioDP = -1;
    int i = 0;
    int FoundPreferred = 0;
    if (Params->DataTransport)
    {
        Svcs->verbose(CP_Stream, "Prefered dataplane name is \"%s\"\n",
                      Params->DataTransport);
    }
    while (List[i].Interface)
    {
        Svcs->verbose(
            CP_Stream,
            "Considering DataPlane \"%s\" for possible use, priority is %d\n",
            List[i].Name, List[i].Priority);
        if (Params->DataTransport)
        {
            if (strcasecmp(List[i].Name, Params->DataTransport) == 0)
            {
                FoundPreferred = 1;
                if (List[i].Priority >= 0)
                {
                    SelectedDP = i;
                    break;
                }
                else
                {
                    fprintf(stderr,
                            "Warning:  Perferred DataPlane \"%s\" is "
                            "not available.",
                            List[i].Name);
                }
            }
        }
        if (List[i].Priority > BestPriority)
        {
            BestPriority = List[i].Priority;
            BestPrioDP = i;
        }
        i++;
    }
    if (Params->DataTransport && (FoundPreferred == 0))
    {
        fprintf(stderr, "Warning:  Preferred DataPlane \"%s\" not found.",
                Params->DataTransport);
    }
    if (SelectedDP != -1)
    {
        Svcs->verbose(CP_Stream,
                      "Selecting DataPlane \"%s\" (preferred) for use\n",
                      List[SelectedDP].Name);
    }
    else
    {
        Svcs->verbose(CP_Stream,
                      "Selecting DataPlane \"%s\", priority %d for use\n",
                      List[BestPrioDP].Name, List[BestPrioDP].Priority);
        SelectedDP = BestPrioDP;
    }
    i = 0;
    while (List[i].Interface)
    {
        if (i != SelectedDP)
        {
            if (List[i].Interface->unGetPriority)
            {
                List[i].Interface->unGetPriority(Svcs, CP_Stream);
            }
        }
        i++;
    }

    if (Params->DataTransport)
    {
        free(Params->DataTransport);
    }
    Params->DataTransport = strdup(List[SelectedDP].Name);

    Ret = List[SelectedDP].Interface;
    free(List);
    return Ret;
}
