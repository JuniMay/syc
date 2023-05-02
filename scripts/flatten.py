import argparse
import os
import shutil

args = None

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input_dir", default="./src")
    parser.add_argument("--output_dir", default="./flattened")
    parser.add_argument("--test", action="store_true", default=False, help="only print the result, not write to file")
    args = parser.parse_args()
    return args


def process(line):
    parts = line.strip().split(" ")
    if parts[0] == "#" and parts[1] == "include":
        print(parts)
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


def dfs(current_path, output_path):
    for file_or_dir in os.listdir(current_path):
        full_path = os.path.join(current_path, file_or_dir)
        if os.path.isfile(full_path) and full_path.endswith(('.cpp', '.h', '.hpp')):
            out_path = os.path.join(output_path, os.path.relpath(full_path, args.input_dir).replace('/', '__'))
            content = ""
            with open(full_path, 'r') as f:
                for line in f:
                    content += process(line)
            print("{:40} -> {:40}".format(full_path, out_path))
            if not args.test:
                with open(out_path, 'w') as f:
                    f.write(content)
        if os.path.isdir(full_path):
            dfs(full_path, output_path)


def run_with(args):
    if os.path.exists(args.output_dir):
        shutil.rmtree(args.output_dir)
    if not os.path.exists(args.output_dir):
        os.mkdir(args.output_dir)
    dfs(args.input_dir, args.output_dir)


if __name__ == '__main__':
    args = parse_args()
    run_with(args)
    # if os.path.exists("./backend"):
    #     shutil.rmtree("./src")
    # if os.path.exists("./testcases"):
    #     shutil.rmtree("./testcases")
    # if os.path.exists("./build"):
    #     shutil.rmtree("./build")
    # if os.path.exists("./third_party"):
    #     shutil.rmtree("./third_party")
    # if os.path.exists("./scripts"):
    #     shutil.rmtree("./scripts")
    # if os.path.exists("./cache"):
    #     shutil.rmtree('./cache')
    # if os.path.exists(".gitignore"):
    #     os.remove(".gitignore")