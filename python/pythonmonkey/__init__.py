from .pythonmonkey import *
#from .__version__ import *
import os
exec(open(os.path.dirname(__file__) + '/version.py').read())
#import importlib.metadata
#__version__= importlib.metadata.version(__name__)
