# @file         python-cjs-module.py
#               A CommonJS Module written Python.
# @author       Wes Garland, wes@distributive.network
# @date         Jun 2023

def helloWorld():
  print('hello, world!')


exports['helloWorld'] = helloWorld
