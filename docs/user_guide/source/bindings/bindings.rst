######################
Language Bindings APIs
######################

Currently ADIOS2 support bindings for the following languages and their minimum standards:

+----------+----------+-------------------------+
| Language | Standard | Interface               |
+----------+----------+-------------------------+
|          | 11       | #include adios2.h       |
| C++      |          |                         |
|          | 98       | #include adios2_cxx98.h |
+----------+----------+-------------------------+
| C        | 99       | #include adios2_c.h     |
+----------+----------+-------------------------+
| Fortran  | 90       | use adios2              |
+----------+----------+-------------------------+
|          | 2.7      | import adios2           |
| Python   |          |                         |
|          | 3        | import adios2           |
+----------+----------+-------------------------+

.. tip::

   Prefer the C++11 bindings if your application C++ compiler supports the 2011 (or later) standard. Only use C++98 APIs for backwards compatiblity with applications that don't allow more recent standard features. C++98 APIs are supported through the C bindings for ABI compatibility.   


.. caution::

   Statically linked libraries (\*.a) might result in conflicting ABIs between C++98 APIs and the C++11 library. Test to make sure the C++98 work for your platform 


The current interaction flow for each language binding API with the ADIOS2 library is specified as follows

.. blockdiag ::
   
   diagram {
      default_fontsize = 22;
      default_shape = roundedbox;
      default_linecolor = blue;
   
      "ADIOS2" -> "C++11", C, Python;
      C -> Fortran, "C++98"; 
      
      "ADIOS2"[width = 200, height = 60]
      
      group{
         label = "C++11 Library"
         color = orange
        "ADIOS2";
      }
      
      group{
         label = "Language bindings APIs"
         color = yellow
         
        "C++11", C, Python, Fortran, "C++98";
      }
      
   }

The following sections provide a summary of the API calls on each language and links to Write and Read examples to put it all together.

.. include:: cxx.rst
.. include:: fortran.rst
.. include:: c.rst
.. include:: python.rst


