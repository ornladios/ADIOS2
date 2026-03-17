***************
Campaign Reader
***************

The Campaign Reader Engine is a specialty engine made for reading Campaign files (.aca) created with `hpc-campaign <https://hpc-campaign.readthedocs.io>`_  tools. See an `example <../advanced/campaign_management.html>`_.

It is an engine, that uses other engine (BP5, HDF5, Timeseries) to open ADIOS/HDF5 datasets added to the campaign file. It directly supports reading images and text files added to the campaign file. 

1. **hostname**: Use this name as local hostname. If the host of a replica of a dataset in the campaign file matches this hostname, it will be regarded as locally readable replica (i.e., its path must be a local path). The default hostname is read from *~/.config/hpc-campaign/config.yaml*.

#. **campaignstorepath**: Use this base path to find campaign files using their relative path/name. Having all campaign files organized under a common base path allows for finding them using a simple relative path under that base path. The default value is read from *~/.config/hpc-campaign/config.yaml*.

#. **cachepath**: Local disk path where extracted pieces from the campaign file are stored, as well as data downloaded by ADIOS when reading remote data. The default value is read from *~/.config/hpc-campaign/config.yaml*.

#. **include-dataset**: As a campaign file contains more and more datasets the open time will increase linearly since ADIOS has to create a reading engine for each BP/HDF5 file, to open that dataset and process it's metadata. If one is only interested in specific datasets in the campaign, a semicolon-separated (;) list of regular expressions can be provided to only open datasets that match one of those expressions.

#. **exclude-dataset**: Similar to the previous option, datasets can be excluded from processing by providing a semicolon-separated list of regular expressions. 

#. **verbose**: print debug information on what is happening when reading a campaign file. 
   
=============================== ===================== ===========================================================
 **Key**                        **Value Format**      **Default** and Examples
=============================== ===================== ===========================================================
 hostname                        string               **hostname** from *~/.config/hpc-campaign/config.yaml*
 campaignstorepath               string               **campaignstorepath** from *~/.config/hpc-campaign/config.yaml*
 cachepath                       string               **cachepath** from *~/.config/hpc-campaign/config.yaml*
 include-dataset                 string               "include-dataset=.*/images;.*/data"
 exclude-dataset                 string               "exclude-dataset=logs/.*"
 verbose                         integer              **0**, 0..5
=============================== ===================== ===========================================================
