These are the script I use to plot json profilers.
In the ADIOS proflier, the important keys are:
   PP PDW ES ES_AWD ES_aggregate_info MetaInfoBcast FixedMetaInfoGather transport_0.wbytes

Therelationship is
   BS (BeginStep)   
   PDW (PerformDataWrite) | PP(PerformPut)
   ES: (EndStep)
    - [ ES_DSD ] optional, if using DataSizeBased Aggregation
    - ES_WriteData 
    - ES_MDAgg
    - ES_CloseTS
   DC (DoClose)

Each section may contain one or more sections.
    
Bytes are reported in "transport_0.wbytes"

For how to plot a json file, see test/commands

* ./writeSummary.sh <f1.json> [<f2.json> .. ]   – produces a high-level summary of write metrics for each JSON file provided.
* python3 plot_json.py <f1.json> [<f2.json> .. ]  - generates quick, informative plots for top level time-measurement tags in the provided JSON files.

The example plots are in ``python_plots/`. All plots reflect per-rank measurements with the BP5 engine. Other engines may omit some or all of these metrics.

*  ES+PDW+PP+BS+DC.png  breaks down of time spent in  ADIOS calls:  EndStep   PerformDataWrite    PeformPut    BeginStep    DoClose   (Total ADIOS I/O impact)
*  ES_DSB+ES_WriteData+ES_MDAgg+ES_CloseTS.png  visualizes how the total EndStep time is broken down, showing the relative contribution of each component/sub-stage.

