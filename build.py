import subprocess
import os, sys
import platform

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

def ensure_spidermonkey():
    # Check if SpiderMonkey libs already exist
    spidermonkey_lib_exist = os.path.exists("./_spidermonkey_install/lib")
    if spidermonkey_lib_exist:
        return

    # Build SpiderMonkey
    execute("bash ./setup.sh")

def build():
    ensure_spidermonkey()
    build_script_sh = os.path.join( dir_path, 'build_script.sh' )
    execute(f"bash {build_script_sh}")
    if platform.system() == "Windows":
        execute("cp ./build/src/*/pythonmonkey.pyd ./python/pythonmonkey/") # Release or Debug build
        execute("cp ./_spidermonkey_install/lib/mozjs-*.dll ./python/pythonmonkey/")
    else:
        execute("cp ./build/src/pythonmonkey.so ./python/pythonmonkey/")
        execute("cp ./_spidermonkey_install/lib/libmozjs* ./python/pythonmonkey/")

if __name__ == "__main__":
    build()
