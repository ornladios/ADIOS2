# Documentation for The Adaptable Input Output System

# Generate User Guide in html with Sphinx

This user guide is hosted in readthedocs: https://adios2.readthedocs.io/en/latest/
 
Python 2.7 or 3 is required to build the Sphinx based user guide. Sphinx can be downloaded using pip `pip install sphinx`. In addition:

1. Required Python packages (all can be installed with `pip install package` or `pip3 install package`):
	* blockdiag https://pypi.python.org/pypi/blockdiag/
	* sphinx v1.4 or above http://www.sphinx-doc.org/en/stable/
	* sphinxcontrib-blockdiag https://pypi.python.org/pypi/sphinxcontrib-blockdiag
	* sphinx_rtd_theme https://sphinx-rtd-theme.readthedocs.io/en/latest/installing.html
	* breathe https://breathe.readthedocs.io/en/latest/
	
2. To generate the User Guide under docs/user_guide/build/html format (default) from the existing Sphinx source files:
	
	```
	$ cd ADIOS2/docs/user_guide 
	$ make html
	```
