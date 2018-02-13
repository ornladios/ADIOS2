# Documentation for the Adaptable Input / Output System (ADIOS) v2.1.0

# Generate Doxygen API Documentation

1. Requirements (on Linux install them from your package manager):
	* doxygen http://www.stack.nl/~dimitri/doxygen/
	* graphviz http://www.graphviz.org/
	
2. To generate the Doxygen API documentation under doc/api_doxygen/html format (default) from the existing Doxyfile (edit Doxyfile to change options):
	
	```
	$ cd ADIOS2/doc/api_doxygen 
	$ doxygen
	``` 

# Generate User Guide in html with Sphinx

Python v2.7 or above is required to build the Sphinx based user guide. Sphinx can be downloaded using pip `pip install sphinx`. In addition:

1. Required Python packages (all can be installed with `pip install package`):
	* blockdiag https://pypi.python.org/pypi/blockdiag/
	* sphinx v1.4 or above http://www.sphinx-doc.org/en/stable/
	* sphinxcontrib-blockdiag: https://pypi.python.org/pypi/sphinxcontrib-blockdiag
	* sphinx_rtd_theme: https://sphinx-rtd-theme.readthedocs.io/en/latest/installing.html
	
2. To generate the User Guide under doc/user_guide/build/html format (default) from the existing Sphinx source files:
	
	```
	$ cd ADIOS2/doc/user_guide 
	$ make html
	```
