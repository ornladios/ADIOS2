# Documentation for The Adaptable Input Output System

# Generate User Guide in html with Sphinx

This user guide is hosted in readthedocs: https://adios2.readthedocs.io/en/latest/

To generate the User Guide under docs/user_guide/build/html format from the Sphinx source files:

```bash
$ cd ADIOS2/docs
docs$ python3 -m venv .
docs$ . bin/activate
docs$ pip3 install -r requirements.txt
user_guide$ cd user_guide
user_guide$ make html
```
