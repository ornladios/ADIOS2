#############################
Simple Language Bindings APIs
#############################

The current simple APIs are designed for simple and direct (not within an application) tasks. They are closely related to the native language IO bindings for formatted text IO. 

.. tip::
   
   Use the simple APIs for very simple tasks in which performance is not a critical aspect:
   * Reading a file to perform data analysis with libraries (matplotlib, scipy, etc.)
   * Interactive: few calls make interactive usage easier
   * Saving data to files is small projects
   
.. note::

   The simpified APIs are based on language native file IO interface. Hence write and read calls are always synchronized and variables are ready to use


Currently ADIOS2 support bindings for the following languages and their minimum standards:

+----------+----------+---------------------+---------------+
| Language | Standard | Interface           | Based on      | 
+----------+----------+---------------------+---------------+
|          | 11/newer | #include adios2.h   |               |
| C++      |          |                     | fstream       |
|          | older    | use C bindings      |               |
+----------+----------+---------------------+---------------+
| C        | 99       | #include adios2_c.h | stdio FILE*   |
+----------+----------+---------------------+---------------+
| Fortran  | 90       | use adios2          | Fortran/stdio |
+----------+----------+---------------------+---------------+
|          | 2.7      | import adios2       | Python        |
| Python   |          |                     | File IO       |
|          | 3        | import adios2       |               |
+----------+----------+---------------------+---------------+

The following sections provide a summary of the API calls on each language and links to Write and Read examples to put it all together.


