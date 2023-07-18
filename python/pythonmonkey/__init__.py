# Export public PythonMonkey APIs
from .pythonmonkey import *
from .require import *

# Expose the package version
import importlib.metadata
__version__= importlib.metadata.version(__name__)

# Load the module by default to make `console`/`atob`/`btoa` globally available
require("console")
require("base64")
