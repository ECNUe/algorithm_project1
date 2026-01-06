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

    #print(f"Compiling {source_file}...")
    cmd = ["g++", source_file, "-o", exe_file, "-O2", "-std=c++11"]
    try:
        subprocess.check_call(cmd)
        return exe_file
    except subprocess.CalledProcessError:
        print(f"Compilation failed for {source_file}")
        return None

def run_test(exe_file, seed):
    # print(f"Running test for {exe_file} with seed {seed}...")
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
    versions = range(4,7) # 0 to 6
    seed = 2013  # Fixed seed for fair comparison
    results = {}

    print(f"Starting tests with seed {seed}...\n")

    for v in versions:
        exe = compile_cpp(v)
        if exe:
            score = run_test(exe, seed)
            results[v] = score
            #print(f"Version {v} Score: {score}\n")
        else:
            results[v] = "Compile Error"

    
    print(f"{'Version':<10} | {'Score':<10}")
    print("-" * 18)
    for v in versions:
        print(f"main{v:<5} | {results[v]:<10}")
    print("-" * 18)

if __name__ == "__main__": 
    versions = range(4, 7) # 4, 5, 6
    num_seeds = 20
    results = {v: [] for v in versions}

    print(f"Starting {num_seeds} benchmark runs for versions 4-6...\n")

    # Compile first
    exes = {}
    for v in versions:
        exe = compile_cpp(v)
        if exe:
            exes[v] = exe
        else:
            print(f"Skipping version {v} due to compilation error.")
    
    # Run tests
    for seed in range(1233, 1233 + num_seeds):
        # print(f"--- Running Seed {seed} ---")
        for v in versions:
            if v in exes:
                score = run_test(exes[v], seed)
                results[v].append(score)
                # print(f"  Version {v}: {score}")

    print("-" * 50)
    print(f"Benchmark Summary (Average Score over {num_seeds} runs):")
    print("-" * 50)
    print(f"{'Version':<10} | {'Average':<10} | {'Min':<10} | {'Max':<10}")
    print("-" * 50)
    for v in versions:
        if results[v]:
            avg = sum(results[v]) / len(results[v])
            min_score = min(results[v])
            max_score = max(results[v])
            print(f"main{v:<5} | {avg:<10.1f} | {min_score:<10} | {max_score:<10}")
        else:
             print(f"main{v:<5} | {'N/A':<10} | {'N/A':<10} | {'N/A':<10}")
    print("-" * 50)