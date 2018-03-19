*******************
Components Overview
*******************

The simple way to understand the big picture for the ADIOS2 core components is to map each class to the actual definition of the ADIOS acronym.

============== =========== ============== ============
 Acronym Word   ADaptable   Input/Output     System   
============== =========== ============== ============
 Component      **ADIOS**      **IO**      **Engine** 
============== =========== ============== ============


ADIOS2's native C++11 application programming interface (API) is fully object-oriented, thus each component is mapped to a class while data is expressed as pointers. Current language bindings will find a more natural API, such as:

============== ======================== ======================
 **Language**      **Component API**     **Application Data**   
============== ======================== ======================
 C++            object/public members    pointers 
 C              handler/functions        pointers 
 Fortran        handler/subroutines      native arrays 
 Python         object/public members    numpy arrays 
============== ======================== ======================


The following figure depicts the components hierarchy from the application's point of view. 

.. image:: http://i63.tinypic.com/33lfe3d.png : alt: my-picture2 

* **ADIOS**: the ADIOS component is the starting point between an application and the ADIOS2 library. Applications provide:   
    1. the scope of the ADIOS object through the MPI communicator, 
    2. a debug mode option for extra exception checking 
    3. an optional runtime configuration file (in XML format) to allow changing settings without recompiling. 
    
    The ADIOS component serves as a factory of adaptable IO components. Each IO must have a unique name within the scope of the ADIOS class object that created them with the DeclareIO function.  

* **IO**: the IO component is the bridge between the application specific settings, transports. It also serves as a factory of: 
    1. Variables 
    2. Attributes
    3. Engines

* **Variables**: Variables are the link between self-describing representation in the ADIOS2 library and data from applications. Variables are identified by unique names in the scope of the particular IO that created them. When the Engine API functions are called, a Variable must be provided along with the application data.

* **Attributes**: an Attributes adds extra information to the overall variables dataset defined in the IO class. They can be single or array values.

* **Engines**: Engines define the actual system executing the heavy IO tasks at Open, BeginStep, Put, Get, EndStep and Close. Due to polymorphism, new IO system solutions can be developed quickly reusing internal components and reusing the same API. The default engine, if IO.SetEngine is not called, is the binary-pack bp file reader and writer: BPFile.

* **Operator**: (under development) this component defines possible operations to be applied on adios2 self-describing data. This higher level abstraction is needed to provide support to: Callback functions, Transforms, Analytics, Data Models functionality, etc. Any required task will be executed within the Engine. One or many operators can be associated with any of the adios2 objects or a group of them.
   