import pytest
import pythonmonkey as pm
import gc

# This is run at the end of each test function


@pytest.fixture(scope="function", autouse=True)
def teardown_function():
  """
  Forcing garbage collection (twice) whenever a test function finishes,
  to locate GC-related errors
  """
  gc.collect(), pm.collect()
  gc.collect(), pm.collect()
