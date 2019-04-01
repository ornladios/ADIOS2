***************************
Runtime Configuration Files
***************************

ADIOS2 supports passing an optional runtime configuration file to the :ref:`ADIOS` component constructor (``adios2_init`` in C, Fortran).

This file contains key-value pairs equivalent to the compile time ``IO::SetParameters`` (``adios2_set_parameter`` in C, Fortran), and ``IO::AddTransport`` (``adios2_set_transport_parameter`` in C, Fortran).

Each Engine should provide a set of available parameters as described in the :ref:`Supported Engines` section. Currently, only the XML format is supported. The typical format is as follows:

.. code-block:: xml

   <?xml version="1.0"?>
   <adios-config>
     <io name="IONAME_1">  

       <engine type="ENGINE_TYPE"> 
            
         <!-- Equivalent to IO::SetParameters--> 
         <parameter key="KEY_1" value="VALUE_1"/>
         <parameter key="KEY_2" value="VALUE_2"/>
         <!-- ... -->
         <parameter key="KEY_N" value="VALUE_N"/> 
        
       </engine>

       <!-- Equivalent to IO::AddTransport -->
       <transport type="TRANSPORT_TYPE">
         <!-- Equivalent to IO::SetParameters--> 
         <parameter key="KEY_1" value="VALUE_1"/>
         <parameter key="KEY_2" value="VALUE_2"/>
         <!-- ... -->
         <parameter key="KEY_N" value="VALUE_N"/>
       </transport>
     </io>
         
     <io name="IONAME_2">  
       <!-- ... -->
     </io>
   </adios-config>
            
           
.. warning::
   
   Only XML files are supported in the current ADIOS2 version. Configuration files must have the ``.xml`` extension: ``config.xml``, ``output.xml``, etc.
