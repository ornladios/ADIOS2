import unittest
import os

class Adios2PythonTestBase(unittest.TestCase):
	### Convenience method to let python tests know if MPI is enabled
	@staticmethod
	def isUsingMpi():
		if 'ADIOS2_PYTHON_TESTS_USE_MPI' in os.environ:
			return True
		return False

	### Expose main method so python tests don't need to import unittest
	@staticmethod
	def main():
		unittest.main()
