from typing import Dict, Any
from abc import abstractmethod
import os
import subprocess

def execute(command) -> Dict[str, Any]:
    """Execute given command.
    Args:
        command: Command to be executed.
    """

    p = subprocess.Popen(command,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    stdout, stderr = p.communicate(timeout=600)
    returncode = p.returncode

    p.terminate()

    return {
        'returncode': returncode,
        'stdout': stdout.decode(),
        'stderr': stderr.decode(),
    }
    
# from https://github.com/JuniMay/yatuner/blob/master/yatuner/compiler.py
class Gcc():
    def __init__(self,
                 src: str,
                 out: str,
                 cc='clang++',
                 template='{cc} {options} {src} -o {out}') -> None:
        self.src = src
        self.out = out
        self.cc = cc
        self.template = template
        self.version = None
        self.params = None
        self.optimizers = None

    def compile(self, options='', src=None, out=None) -> None:

        if src is not None:
            self.src = src

        if out is not None:
            self.out = out

        command = self.template.format(cc=self.cc,
                                       options=options,
                                       out=self.out,
                                       src=self.src)
        print("command: ", command)
        res = execute(command)

        if res['returncode'] != 0:
            raise RuntimeError(res['stderr'])

    def fetch_execute_cmd(self) -> str:
        return self.out

in_file = os.listdir("./flattened")
# find all that ends with .cpp
in_file = [os.path.join("./flattened", x) for x in in_file if x.endswith(".cpp")]

gcc = Gcc(src=' '.join(in_file), out='syc')
gcc.compile("--std=c++17 -O2 -lm -I ./flattened")