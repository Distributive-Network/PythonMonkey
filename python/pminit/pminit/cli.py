import os, sys
import subprocess
import argparse

def execute(cmd: str, cwd: str):
    popen = subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr = subprocess.STDOUT,
        shell = True, text = True, cwd = cwd )
    for stdout_line in iter(popen.stdout.readline, ""):
        sys.stdout.write(stdout_line)
        sys.stdout.flush()

    popen.stdout.close()
    return_code = popen.wait()
    #For some reason npm returns a non zero
    #exit code when `--help` is used in the commands.
    #This shouldn't raise an error.
    if return_code and "--help" not in cmd:
        raise subprocess.CalledProcessError(return_code, cmd)

def commandType(value: str):
    if value != "npm":
        raise argparse.ArgumentTypeError("Value must be npm.")
    return value

def main():
    parser = argparse.ArgumentParser(description="A tool to enable running npm on the correct package.json location")
    parser.add_argument("executable", nargs=1, help="Should be npm.", type=commandType)
    parser.add_argument("args", nargs = argparse.REMAINDER)
    args = parser.parse_args()
  
    pythonmonkey_path= os.path.realpath(
        os.path.join(
            os.path.dirname(__file__),
            '..',
            'pythonmonkey'
        )
    )

    execute(' '.join( args.executable + args.args ), pythonmonkey_path)

    
    
    
