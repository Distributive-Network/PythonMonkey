import subprocess
import os, sys
import platform

dir_path = os.path.dirname( os.path.realpath(__file__) )
system = platform.system()


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

def build():
    build_script_sh = os.path.join( dir_path, 'build_script.sh' )
    execute(f"bash {build_script_sh}")
    execute("cp ./build/src/pythonmonkey.so ./python/pythonmonkey/")
    if system == "Linux":
        execute("cp ./_spidermonkey_install/lib/libmozjs-102.so ./python/pythonmonkey/")
    elif system == "Darwin":
        execute("cp ./_spidermonkey_install/lib/libmozjs-102.dylib ./python/pythonmonkey/")
    else: # system == "Windows"
        raise NotImplementedError("Windows is not supported yet.")

if __name__ == "__main__":
    build()
