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

def build():
    build_script_sh = os.path.join( dir_path, 'build_script.sh' )
    execute(f"bash {build_script_sh}")
    execute("cp ./build/src/pythonmonkey.so ./python/pythonmonkey/")
    execute("cp ./_spidermonkey_install/lib/libmozjs* ./python/pythonmonkey/")

if __name__ == "__main__":
    build()
