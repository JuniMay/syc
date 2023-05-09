import argparse
import os
import shutil
import subprocess
from typing import Any, Dict


def execute(command, timeout) -> Dict[str, Any]:
    p = subprocess.Popen(command,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)

    try:
        stdout, stderr = p.communicate(timeout=timeout)
        returncode = p.returncode

        p.terminate()

        return {
            'returncode': returncode,
            'stdout': stdout.decode(),
            'stderr': stderr.decode(),
        }

    except subprocess.TimeoutExpired:
        return {'returncode': -1, 'stdout': '', 'stderr': 'TIMEOUT'}


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--src-dir', default='./src')
    parser.add_argument('--output-dir', default='./output')
    parser.add_argument('--flatten-dir', default='./flattened')
    parser.add_argument('--testcase-dir', default='./tests')
    parser.add_argument('--runtime-lib-dir', default='./sysy-runtime-lib')

    parser.add_argument('--executable-path', default='./syc')

    parser.add_argument('--no-compile', action='store_true', default=False)
    parser.add_argument('--no-flatten', action='store_true', default=False)
    parser.add_argument('--no-test', action='store_true', default=False)

    return parser.parse_args()


def process_line(line: str) -> str:
    parts = line.strip().split(" ")
    if parts[0] == "#" and parts[1] == "include":
        if parts[2]:
            hd = parts[2].replace("/", "__")
        else:
            hd = parts[3].replace("/", "__")
        return "#include " + hd + "\n"
    if len(parts) != 2:
        return line
    if parts[0] != "#include":
        return line
    if "sys/" in parts[1]:
        return line
    hd = parts[1].replace("/", "__")
    return "#include " + hd + "\n"


def flatten_src(curr_path: str, input_dir: str, output_path: str):
    for file_or_dir in os.listdir(curr_path):
        full_path = os.path.join(curr_path, file_or_dir)
        if os.path.isfile(full_path) and full_path.endswith(
            ('.cpp', '.h', '.hpp')):

            out_path = os.path.join(
                output_path,
                os.path.relpath(full_path, input_dir).replace('/', '__'))
            content = ""

            with open(full_path, 'r') as f:
                for line in f:
                    content += process_line(line)

            print("{:40} -> {:40}".format(full_path, out_path))

            with open(out_path, 'w') as f:
                f.write(content)

        # duplicate generated parser.h to avoid header problem.
        if os.path.isfile(full_path) and full_path.endswith('parser.h'):
            out_path = os.path.join(output_path, 'parser.h')
            content = ""

            with open(full_path, 'r') as f:
                for line in f:
                    content += process_line(line)

            print("{:40} -> {:40}".format(full_path, out_path))

            with open(out_path, 'w') as f:
                f.write(content)

        if os.path.isdir(full_path):
            flatten_src(full_path, input_dir, output_path)


def compile(include_dir: str, executable_path: str, timeout: int):
    in_file = os.listdir(include_dir)

    in_file = [
        os.path.join(include_dir, x) for x in in_file if x.endswith(".cpp")
    ]

    in_file_str = ' '.join(in_file)
    command = (f'clang++ --std=c++17 -O2 -lm '
               f'-I{include_dir} {in_file_str} -o {executable_path}')

    print(f'BUILDING: {command}')
    result = execute(command, timeout)

    print(result['stdout'])

    if result['returncode'] != 0:
        print('BUILDING FAILED.\n')
        print(result['stderr'])
        exit(1)
    else:
        print('BUILDING FINISHED.\n')


def test(executable_path: str, testcase_dir: str, output_dir: str,
         runtime_lib_dir: str, exec_timeout: int):
    testcase_list = []

    def dfs(curr_dir: str):
        dir_list = sorted(os.listdir(curr_dir))

        for file_or_dir in dir_list:
            full_path = os.path.join(curr_dir, file_or_dir)

            if os.path.isfile(full_path) and full_path.endswith('.sy'):
                testcase_list.append(full_path.rsplit('.', 1)[0])

            elif os.path.isdir(full_path):
                dfs(full_path)
                
    dfs(testcase_dir)

    for testcase in testcase_list:
        basename : str =  os.path.basename(testcase)

        tokens_path = os.path.join(output_dir, f'{basename}.toks')
        ast_path = os.path.join(output_dir, f'{basename}.ast')
        ir_path = os.path.join(output_dir, f'{basename}.ll')
        asm_path = os.path.join(output_dir, f'{basename}.asm')

        std_asm_from_ir_path = os.path.join(output_dir,
                                            f'{basename}.std_from_ir.asm')

        std_ir_path = os.path.join(output_dir, f'{basename}.std.ll')
        std_asm_path = os.path.join(output_dir, f'{basename}.std.asm')
        log_path = os.path.join(output_dir, f'{basename}.log')

        log_file = open(log_path, 'w')

        command = (f'{executable_path} {testcase}.sy -S '
                   f'-o {asm_path} '
                   f'--emit-tokens {tokens_path} '
                   f'--emit-ast {ast_path} '
                   f'--emit-ir {ir_path} ')

        exec_result = execute(command, exec_timeout)

        log_file.write(f'EXECUTE: {command}\n')
        log_file.write(f'STDOUT:\n')
        log_file.write(exec_result['stdout'])
        log_file.write(f'STDERR:\n')
        log_file.write(exec_result['stderr'])

        if exec_result['returncode'] == 0:
            print(f'(syc) [ SUCCESS ] {testcase}')

        else:
            if exec_result['stderr'] == 'TIMEOUT':
                print(f'(syc) [ TIMEOUT ] {testcase}')
            else:
                print(f'(syc) [  ERROR  ] {testcase}')

        command = (f'clang -xc {testcase}.sy -include '
                   f'{runtime_lib_dir}/sylib.h '
                   f'-S -emit-llvm -o {std_ir_path}')

        exec_result = execute(command, exec_timeout)

        log_file.write(f'EXECUTE: {command}\n')
        log_file.write(f'STDOUT:\n')
        log_file.write(exec_result['stdout'])
        log_file.write(f'STDERR:\n')
        log_file.write(exec_result['stderr'])
        
        command = (f'clang -xc {testcase}.sy -include '
                   f'{runtime_lib_dir}/sylib.h '
                   f'-S --target=riscv64 -o {std_asm_path}')

        exec_result = execute(command, exec_timeout)

        log_file.write(f'EXECUTE: {command}\n')
        log_file.write(f'STDOUT:\n')
        log_file.write(exec_result['stdout'])
        log_file.write(f'STDERR:\n')
        log_file.write(exec_result['stderr'])

        command = (f'llc {ir_path} -o '
                   f'{std_asm_from_ir_path} '
                   f'--march=riscv64')

        exec_result = execute(command, exec_timeout)

        log_file.write(f'EXECUTE: {command}\n')
        log_file.write(f'STDOUT:\n')
        log_file.write(exec_result['stdout'])
        log_file.write(f'STDERR:\n')
        log_file.write(exec_result['stderr'])


def main():
    args = parse_args()

    if not args.no_flatten:
        if os.path.exists(args.flatten_dir):
            shutil.rmtree(args.flatten_dir)

        if not os.path.exists(args.flatten_dir):
            os.makedirs(args.flatten_dir)

        flatten_src(args.src_dir, args.src_dir, args.flatten_dir)

    if not args.no_compile:
        compile(args.flatten_dir, args.executable_path, args.timeout)

    if not args.no_test:
        if os.path.exists(args.output_dir):
            shutil.rmtree(args.output_dir)

        if not os.path.exists(args.output_dir):
            os.makedirs(args.output_dir)

        test(args.executable_path, args.testcase_dir, args.output_dir,
             args.runtime_lib_dir, args.timeout)


if __name__ == '__main__':
    main()