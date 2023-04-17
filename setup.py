from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext as _build_ext
import subprocess
import os, sys

dir_path = os.path.dirname( os.path.realpath(__file__) )


def execute(cmd: str):
    popen = subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr = subprocess.STDOUT,
        shell = True, text = True )
    for stdout_line in iter(popen.stdout.readline, ""):
        sys.stdout.write(stdout_line)
        sys.stdout.flush()

    popen.stdout.close()
    return_code = popen.wait()
    if return_code:
        raise subprocess.CalledProcessError(return_code, cmd)


class build_ext(): #_build_ext):
    def __init__(self): #run(self):
        setup_sh = os.path.join( dir_path, 'setup.sh ') 
        build_script_sh = os.path.join( dir_path, 'build_script.sh' )

        #execute(f"bash {setup_sh}")
        execute(f"bash {build_script_sh}")
build_ext()

#with open( os.path.join( dir_path, 'version.txt' ), 'r' ) as f:
#    version = "".join(f.readlines())
#
#version = version.split(".")
#
#if len(version) > 3:
#    version = ".".join( version[:3] )
#elif len(version) !=3:
#    raise ValueError(f"Version in version.txt is incorrect semver for python packages: {'.'.join(version)}")
#else:
#    version = ".".join( version )
#
#
#
#setup(
#    cmdclass={"build_ext": build_ext},
#    version = version,
#    packages = find_packages(),
#    package_data = { '
#)
