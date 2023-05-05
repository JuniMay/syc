import os
import subprocess
from typing import Dict, Any
import shutil


def execute(command) -> Dict[str, Any]:
    """Execute given command.
    Args:
        command: Command to be executed.
    """

    p = subprocess.Popen(command,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)

    try:
        stdout, stderr = p.communicate(timeout=100)
        returncode = p.returncode

        p.terminate()

        return {
            'returncode': returncode,
            'stdout': stdout.decode(),
            'stderr': stderr.decode(),
        }
    except subprocess.TimeoutExpired:
        return {'returncode': -1, 'stdout': '', 'stderr': 'TIMEOUT'}


def compile(flattened_dir: str):
    in_file = os.listdir(flattened_dir)
    # find all that ends with .cpp
    in_file = [
        os.path.join(flattened_dir, x) for x in in_file if x.endswith(".cpp")
    ]

    in_file_str = ' '.join(in_file)
    command = (f'clang++ --std=c++17 -O2 -lm '
               f'-I{flattened_dir} {in_file_str} -o syc')

    print(f'BUILDING: {command}')
    result = execute(command)

    print(result['stdout'])

    if result['returncode'] != 0:
        print(f'FAILED TO BUILD')
        print(result['stderr'])
        exit(1)
    else:
        print(f'FINISHED BUILDING')


if __name__ == '__main__':
    compile('flattened')