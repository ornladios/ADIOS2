These are the script I use to plot json profilers.
In the ADIOS proflier, the important keys are:
   PP PDW ES ES_AWD ES_aggregate_info MetaInfoBcast FixedMetaInfoGather transport_0.wbytes

The relationship of the sections  is
   BS (BeginStep)   
   PDW (PerformDataWrite) | PP(PerformPut)
   ES: (EndStep), main subsections are:
    - [ ES_DSD ] optional, if using DataSizeBased Aggregation
    - ES_WriteData 
    - ES_MDAgg
    - ES_CloseTS
   DC (DoClose)

Each sub/section may contain one or more subsections.
    
Bytes are reported in "transport_0.wbytes"

For how to plot a json file, see test/commands

* ./writeSummary.sh <f1.json> [<f2.json> .. ]   – produces a high-level summary of write metrics for each JSON file provided. e.g.

```bash
 ./writeSummary.sh data/v/v.json
=======  High level Summary of data/v/v.json =======
Num Ranks: 4. NumSteps: 10
   DataSizeBased + SelectiveAggregationMetadata
      t1  bytes: 202.56 KiB
      t0  bytes: 12.50 GiB
   Max [ES + PDW]: = 1.602 sec at rank: 3
   Min [ES + PDW]: = 1.592 sec at rank: 2
```



* python3 plot_json.py <f1.json> [<f2.json> .. ]  - generates quick, informative plots for top level time-measurement tags in the provided JSON files.

The example plots are in ``python_plots/`. All plots reflect per-rank measurements with the BP5 engine. Other engines may omit some or all of these metrics.

*  ES+PDW+PP+BS+DC.png  breaks down of time spent in  ADIOS calls:  EndStep   PerformDataWrite    PeformPut    BeginStep    DoClose   (Total ADIOS I/O impact)
*  ES_DSB+ES_WriteData+ES_MDAgg+ES_CloseTS.png  visualizes how the total EndStep time is broken down, showing the relative contribution of each component/sub-stage.

