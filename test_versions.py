import subprocess
import os
import sys
import re

def compile_cpp(version):
    source_file = f"versions/main{version}.cpp"
    exe_file = f"versions/main{version}.exe"
    
    if not os.path.exists(source_file):
        print(f"Source file not found: {source_file}")
        return None

    print(f"Compiling {source_file}...")
    cmd = ["g++", source_file, "-o", exe_file, "-O2", "-std=c++11"]
    try:
        subprocess.check_call(cmd)
        return exe_file
    except subprocess.CalledProcessError:
        print(f"Compilation failed for {source_file}")
        return None

def run_test(exe_file, seed):
    print(f"Running test for {exe_file} with seed {seed}...")
    cmd = ["python", "judge.py", exe_file, str(seed)]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        output = result.stdout
        
        # Extract score
        match = re.search(r"Final Score: (\d+)", output)
        if match:
            return int(match.group(1))
        else:
            print("Could not find score in output.")
            # print(output) # Debug
            return 0
    except Exception as e:
        print(f"Error running test: {e}")
        return 0

def main():
    versions = range(7) # 0 to 6
    seed = 288 # Fixed seed for fair comparison
    results = {}

    print(f"Starting benchmark with seed {seed}...\n")

    for v in versions:
        exe = compile_cpp(v)
        if exe:
            score = run_test(exe, seed)
            results[v] = score
            print(f"Version {v} Score: {score}\n")
        else:
            results[v] = "Compile Error"

    print("-" * 30)
    print("Benchmark Results:")
    print("-" * 30)
    print(f"{'Version':<10} | {'Score':<10}")
    print("-" * 30)
    for v in versions:
        print(f"main{v:<5} | {results[v]:<10}")
    print("-" * 30)

if __name__ == "__main__":
    main()
