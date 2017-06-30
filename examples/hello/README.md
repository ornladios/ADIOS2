examples/hello 

Provides tests and illustrates how to use very basic functionality in adios2

1. adios1Writer (-DADIOS2_USE_ADIOS1=ON -DADIOS1_DIR=/adios1-location)  
    * Write BP format files using adios1 library through adios2 interface    
	
1. bpWriter    
    * Write BP format files for one Variable  
    
1. bpTimeWriter
    * Write BP format files for two Variables (one is timestep) using time aggregation   
    
1. datamanReader (to be deprecated, ADIOS_USE_DataMan=ON)
    * Read real-time WAN streams using dataman 
    
1. datamanWriter  
    * Write real-time WAN streams using dataman    
    
1. hdf5Writer (-DADIOS_USE_HDF5=ON)  
    * Write HDF5 files using interoperability through the adios2 interface    