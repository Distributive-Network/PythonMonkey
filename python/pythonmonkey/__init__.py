# Export public PythonMonkey APIs
from .pythonmonkey import *
from .require import *

# Expose the package version
import importlib.metadata
__version__= importlib.metadata.version(__name__)

# Load the module by default to expose global APIs
require("console")
require("base64")
require("timers")
