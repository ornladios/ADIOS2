These are the script I use to plot json profilers.
In the ADIOS proflier, the important keys are:
   PP PDW ES ES_AWD ES_aggregate_info MetaInfoBcast FixedMetaInfoGather transport_0.wbytes

The relationship is
   PDW (PerformDataWrite) | PP(PerformPut)
   ES: (EndStep)
    - ES_AWD
    - ES_aggregate_info
       - FixedMetaInfoGather
       - MetaInfoBcase
   Bytes are reported in "transport_0.wbytes"


For how to plot a json file, see test/commands
