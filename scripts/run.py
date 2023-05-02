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


def dfs(exec_path: str, test_dir: str, output_dir: str):
    paths = os.listdir(test_dir)

    for file_or_dir in paths:
        full_path = os.path.join(test_dir, file_or_dir)

        if os.path.isfile(full_path) and full_path.endswith('sy'):
            basename = os.path.splitext(os.path.basename(full_path))[0]

            output_ast_path = os.path.join(output_dir, basename + '.ast')
            output_token_path = os.path.join(output_dir, basename + '.toks')
            output_ir_path = os.path.join(output_dir, basename + '.ll')
            outpur_asm_path = os.path.join(output_dir, basename + '.asm')

            command = (f'{exec_path} {full_path} -S -o {outpur_asm_path} '
                       f'--emit-ast {output_ast_path} '
                       f'--emit-tokens {output_token_path} '
                       f'--emit-ir {output_ir_path}')

            exec_result = execute(command)

            log_path = os.path.join(output_dir, basename + '.log')

            with open(log_path, 'w') as f:
                f.write(command)
                f.write('\n')
                f.write(exec_result['stdout'])
                f.write('\n')
                f.write(exec_result['stderr'])
                f.write('\n')

            if (exec_result['returncode'] != 0):
                print(f'[  ERROR  ] {full_path}')
            else:
                print(f'[ SUCCESS ] {full_path}')

        elif os.path.isdir(full_path):
            dfs(exec_path, full_path, output_dir)


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
    else:
        print(f'FINISHED BUILDING')


def run(exec_path: str, test_dir: str, output_dir: str):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    dfs(exec_path, test_dir, output_dir)


if __name__ == '__main__':
    compile('./flattened')

    if os.path.exists('./tests_output'):
        shutil.rmtree('./tests_output')

    run('./syc', './tests', './tests_output')
