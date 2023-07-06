import subprocess
import os, sys
import platform

TOP_DIR = os.path.abspath(os.path.dirname(__file__))
BUILD_DIR = os.path.join(TOP_DIR, "build")

# Get number of CPU cores
CPUS = os.cpu_count() or 1

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

def ensure_spidermonkey():
    # Check if SpiderMonkey libs already exist
    spidermonkey_lib_exist = os.path.exists("./_spidermonkey_install/lib")
    if spidermonkey_lib_exist:
        return

    # Build SpiderMonkey
    execute("bash ./setup.sh")

def run_cmake_build():
    os.makedirs(BUILD_DIR, exist_ok=True) # mkdir -p
    os.chdir(BUILD_DIR)
    if platform.system() == "Windows":
        execute("cmake .. -T ClangCL") # use Clang/LLVM toolset for Visual Studio
    else:
        execute("cmake ..")
    execute(f"cmake --build . -j{CPUS} --config Release")
    os.chdir(TOP_DIR)

def copy_artifacts():
    if platform.system() == "Windows":
        execute("cp ./build/src/*/pythonmonkey.pyd ./python/pythonmonkey/") # Release or Debug build
        execute("cp ./_spidermonkey_install/lib/mozjs-*.dll ./python/pythonmonkey/")
    else:
        execute("cp ./build/src/pythonmonkey.so ./python/pythonmonkey/")
        execute("cp ./_spidermonkey_install/lib/libmozjs* ./python/pythonmonkey/")

def build():
    ensure_spidermonkey()
    run_cmake_build()
    copy_artifacts()

if __name__ == "__main__":
    build()
