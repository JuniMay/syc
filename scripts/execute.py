import argparse
import difflib
import os
import shutil
import subprocess
from typing import Any, Dict


def check_file(file1, file2, diff_file):
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        diff = difflib.unified_diff(
            list(map(lambda x: x.strip(), f1.readlines())),
            list(map(lambda x: x.strip(), f2.readlines())),
            fromfile=file1,
            tofile=file2,
        )
        diff_list = list(diff)

        with open(diff_file, 'w') as f:
            f.writelines(diff_list)

        return len(diff_list) == 0


def execute(command, timeout) -> Dict[str, Any]:
    try:
        result = subprocess.run(command,
                                shell=True,
                                timeout=timeout,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True)

        return {
            'returncode': result.returncode,
            'stdout': result.stdout,
            'stderr': result.stderr,
        }
    except Exception as e:
        return {'returncode': None, 'stdout': '', 'stderr': str(e)}


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--src-dir', default='./src')
    parser.add_argument('--output-dir', default='./output')
    parser.add_argument('--flatten-dir', default='./flattened')
    parser.add_argument('--testcase-dir', default='./tests/functional')
    parser.add_argument('--runtime-lib-dir', default='./sysy-runtime-lib')

    parser.add_argument('--executable-path', default='./syc')

    parser.add_argument('--no-compile', action='store_true', default=False)
    parser.add_argument('--no-flatten', action='store_true', default=False)
    parser.add_argument('--no-test', action='store_true', default=False)
    
    parser.add_argument('--llm', action='store_true', default=False)

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


def log(logfile, command, exec_result):
    logfile.write(f'EXECUTE: {command}\n')
    logfile.write(f'STDOUT:\n')
    logfile.write(exec_result['stdout'])
    logfile.write(f'STDERR:\n')
    logfile.write(exec_result['stderr'])
    logfile.write(f'\n')


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
        basename: str = os.path.basename(testcase)

        in_path = f'{testcase}.in'

        std_out_path = f'{testcase}.out'
        if not os.path.isfile(in_path):
            in_path = None

        tokens_path = os.path.join(output_dir, f'{basename}.toks')
        ast_path = os.path.join(output_dir, f'{basename}.ast')
        ir_path = os.path.join(output_dir, f'{basename}.ll')
        asm_path = os.path.join(output_dir, f'{basename}.s')
        obj_from_ir_path = os.path.join(output_dir, f'{basename}.ir.o')
        obj_from_asm_path = os.path.join(output_dir, f'{basename}.asm.o')
        out_path = os.path.join(output_dir, f'{basename}.out')
        exec_path = os.path.join(output_dir, f'{basename}')

        diff_path = os.path.join(output_dir, f'{basename}.diff')

        std_asm_from_ir_path = os.path.join(output_dir,
                                            f'{basename}.std_from_ir.s')

        std_ir_path = os.path.join(output_dir, f'{basename}.std.ll')
        std_asm_path = os.path.join(output_dir, f'{basename}.std.s')

        log_path = os.path.join(output_dir, f'{basename}.log')
        log_file = open(log_path, 'w')

        command = (f'{executable_path} {testcase}.sy -S '
                   f'-o {asm_path} '
                   f'--emit-tokens {tokens_path} '
                   f'--emit-ast {ast_path} '
                   f'--emit-ir {ir_path} ')

        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        if exec_result['returncode'] is None:
            if exec_result['stderr'] == 'TIMEOUT':
                print(f'[ TIMEOUT ] (syc) {testcase}')
            else:
                print(f'[  ERROR  ] (syc) {testcase}')

            continue

        command = (f'clang -xc {testcase}.sy -include '
                   f'{runtime_lib_dir}/sylib.h '
                   f'-S -emit-llvm -o {std_ir_path}')

        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        command = (f'clang -S --target=riscv64 -mabi=lp64d {ir_path} '
                   f'-o {std_asm_from_ir_path}')

        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        if exec_result['returncode'] is None:
            print(f'[  ERROR  ] (ir->asm) {testcase}')
            continue

        command = (f'clang -fPIC -c --target=riscv64 -mabi=lp64d {ir_path} '
                   f'-o {obj_from_ir_path}')

        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        if exec_result['returncode'] is None:
            print(f'[  ERROR  ] (ir->obj) {testcase}')
            continue
        
        command = (f'clang -fPIC -c --target=riscv64 -mabi=lp64d {asm_path} '
                   f'-o {obj_from_asm_path}')

        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        if exec_result['returncode'] is None:
            print(f'[  ERROR  ] (asm->obj) {testcase}')
            continue

        # command = (f'riscv64-linux-gnu-gcc -march=rv64gc {obj_from_ir_path}'
        #            f' -L{runtime_lib_dir} -lsylib -o {exec_path}')
        
        command = (f'riscv64-linux-gnu-gcc -march=rv64gc {obj_from_asm_path}'
                   f' -L{runtime_lib_dir} -lsylib -o {exec_path}')

        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        if exec_result['returncode'] is None:
            print(f'[  ERROR  ] (gcc) {testcase}')

        command = (f'qemu-riscv64 -L /usr/riscv64-linux-gnu {exec_path}'
                   f' >{out_path}') if in_path is None else (
                       f'qemu-riscv64 -L /usr/riscv64-linux-gnu {exec_path}'
                       f' <{in_path} >{out_path}')

        exec_result = execute(command, exec_timeout)

        need_newline = False
        with open(out_path, 'r') as f:
            content = f.read()
            if len(content) > 0:
                if not content.endswith('\n'):
                    need_newline = True
        # add return code to the last line of out file
        with open(out_path, 'a+') as f:
            if need_newline:
                f.write('\n')
            f.write(str(exec_result['returncode']))
            f.write('\n')

        is_equal = check_file(out_path, std_out_path, diff_path)

        if is_equal:
            print(f'[ CORRECT ] {testcase}')
        else:
            print(f'[  ERROR  ] (WA) {testcase}')

        log(log_file, command, exec_result)

# put all files together in an .txt file with their relative path
def flatten4llm(dir_path: str, flatten_dir_path: str):
    # 指定需要的文件扩展名
    extensions = ['.h', '.cpp', '.y', '.l']
    output_file_path = os.path.join(flatten_dir_path, "llm.txt")

    with open(output_file_path, 'w') as output_file:
        # 遍历根目录下的所有目录和文件
        for foldername, subfolders, filenames in os.walk(dir_path):
            # 排除包含generated的路径
            if 'generated' in foldername:
                continue
            
            for filename in filenames:
                # 使用endswith过滤文件
                if any(filename.endswith(ext) for ext in extensions):
                    # 获取文件的相对路径
                    relative_path = os.path.join(foldername, filename)
                    with open(relative_path, 'r') as source_file:
                        try:
                            source_code = source_file.read()
                            # 在txt文件中写入文件的相对路径和源代码
                            output_file.write(f'Path: {relative_path}\n{source_code}\n\n')
                        except Exception as e:
                            print(f"Error reading file {relative_path}: {e}")


def main():
    args = parse_args()

    if args.llm:
        if os.path.exists(args.flatten_dir):
            shutil.rmtree(args.flatten_dir)

        if not os.path.exists(args.flatten_dir):
            os.makedirs(args.flatten_dir)
        
        flatten4llm(args.src_dir, args.flatten_dir)
        
        return

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