import subprocess
import sys
import shutil

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

def main():
    node_package_manager = 'npm'
    # check if npm is installed on the system
    if (shutil.which(node_package_manager) is None):
        print("""

PythonMonkey Build Error:

    
  *    It appears npm is not installed on this system.
  *    npm is required for PythonMonkey to build.
  *    Please install NPM and Node.js before installing PythonMonkey.
  *    Refer to the documentation for installing NPM and Node.js here: https://nodejs.org/en/download


        """)
        raise Exception("PythonMonkey build error: Unable to find npm on the system.")
    else:
        execute(f"cd pythonmonkey && {node_package_manager} i --no-package-lock") # do not update package-lock.json

if __name__ == "__main__":
    main()

