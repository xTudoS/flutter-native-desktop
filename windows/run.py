import subprocess
import sys

def run_command(cmd):
    cmd = subprocess.Popen(cmd.split(" "), stdout=subprocess.PIPE, shell=True)
    return cmd.stdout.read().decode().strip()

if __name__ == "__main__":
    cmd = sys.argv[1]
    print(run_command(cmd))