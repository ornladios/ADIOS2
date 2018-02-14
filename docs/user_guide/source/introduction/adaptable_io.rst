*******************************************************
Adaptable IO beyond files in Scientific Data Lifecycles
*******************************************************

Exascale computing, Big Data, Internet of Things (IoT), Burst Buffers, High Bandwidth Memory (HBM), Remote Direct Memory Access (RDMA)...all these novel terms and technologies have one thing in common: Data Management at large scales has become more relevant when making informed decisions. The goal of ADIOS2 is to provide an **adaptable**, **scalable**, and **unified** framework to aid scientific applications in their data transfer needs going beyond file input/output (IO) storage. As shown in the figure, there are many data management challenges in the scientific data life cycle beyond file I/O that ADIOS2 aspires to address 

.. image:: http://i65.tinypic.com/2h5k38w.png : alt: my-picture1 


In that regard, we are attempting to create a framework to provide:


* Custom application management of massive data sets from generation, analysis, movement, to short-term and long-term storage.
  
* Self-describing data in binary-packed (BP) format for quick information extraction

* Ability to separate and extract relevant information from large data sets for better decision making
  
* Ability to make near real-time decisions based on in-transit or in-situ analytics

* Expand to other transport mechanisms with minimal overhead to the user: Wide-Area-Network (WAN), Remote Direct Memory Access (ibverbs, NVLink, etc.), Shared-Memory
  
* Exploit new memory hierarchy and scalability in novel hardware architectures: HBM, Burst Buffers, many-cores, etc.
