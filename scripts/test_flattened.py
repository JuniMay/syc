import os
import subprocess
from typing import Any, Dict


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

in_file = os.listdir("./flattened")
# find all that ends with .cpp
in_file = [
    os.path.join("./flattened", x) for x in in_file if x.endswith(".cpp")
]

in_file_str = ' '.join(in_file)
command = f'clang++ --std=c++17 -O2 -lm -I./flattened {in_file_str} -o syc'
print(command)
execute(command)