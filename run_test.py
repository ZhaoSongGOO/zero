import os
import subprocess
import sys
import difflib

# Configuration
TEST_DIR = "./test"
EXECUTABLE = "./out/zero"

def get_focused_diff(expected, actual, filename, label):
    """Generates a git-style unified diff."""
    expected_lines = expected.splitlines(keepends=True)
    actual_lines = actual.splitlines(keepends=True)
    
    diff = difflib.unified_diff(
        expected_lines, 
        actual_lines, 
        fromfile=f"{filename} (baseline)", 
        tofile=f"{filename} (actual {label})",
        n=2  # Number of context lines around the change
    )
    return "".join(diff)

def compare_and_report(name, actual, base_path, extension):
    """Handles comparison and generates focused diffs on failure."""
    # Normalize actual output to ensure consistency
    actual = actual.strip()
    
    if not os.path.exists(base_path):
        with open(base_path, "w", encoding="utf-8") as f:
            f.write(actual)
        print(f"[ NEW {extension} BASELINE ]", end=" ")
        return "generated"
    
    with open(base_path, "r", encoding="utf-8") as f:
        expected = f.read().strip()

    if actual == expected:
        return "passed"
    else:
        # Generate focused diff instead of printing everything
        diff_output = get_focused_diff(expected, actual, name, extension)
        print(f"\n\n❌ {extension} DIFF DETECTED for {name}:")
        print("=" * 50)
        print(diff_output if diff_output else "Content differs in whitespace/encoding.")
        print("=" * 50)
        return "failed"

def run_tests():
    if not os.path.exists(EXECUTABLE):
        print(f"Error: Executable '{EXECUTABLE}' not found.")
        return

    test_files = sorted([f for f in os.listdir(TEST_DIR) if f.endswith(".z")])
    stats = {"passed": 0, "failed": 0, "generated": 0}

    print(f"--- Zero Lang: Dual-Stage Testing (Stdout & IR) ---")

    for file_name in test_files:
        base_name = os.path.splitext(file_name)[0]
        z_path = os.path.join(TEST_DIR, file_name)
        ir_source_path = os.path.join(TEST_DIR, f"{base_name}.ir")
        
        out_base_path = os.path.join(TEST_DIR, f"{base_name}.out.base")
        ir_base_path = os.path.join(TEST_DIR, f"{base_name}.ir.base")

        print(f"Testing {file_name:20}...", end=" ", flush=True)

        try:
            result = subprocess.run(
                [EXECUTABLE, z_path],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            # Stage 1: Stdout
            out_status = compare_and_report(file_name, result.stdout, out_base_path, "OUT")
            
            # Stage 2: IR
            ir_status = "passed"
            if os.path.exists(ir_source_path):
                with open(ir_source_path, "r", encoding="utf-8") as ir_file:
                    ir_status = compare_and_report(file_name, ir_file.read(), ir_base_path, "IR")
            else:
                # If IR file is missing but .z exists, we consider it a failure 
                # or a skip depending on your preference.
                print(f"[ SKIP IR ]", end=" ")

            if out_status == "failed" or ir_status == "failed":
                stats["failed"] += 1
            elif out_status == "generated" or ir_status == "generated":
                stats["generated"] += 1
                print("✅")
            else:
                stats["passed"] += 1
                print("✅")
            
        except subprocess.TimeoutExpired:
            print("⏰ TIMEOUT")
            stats["failed"] += 1
        except Exception as e:
            print(f"⚠️ ERROR: {e}")
            stats["failed"] += 1

    print("\n" + "="*45)
    print(f"Final: {stats['passed']} Passed | {stats['failed']} Failed | {stats['generated']} New")
    print("="*45)

    if stats["failed"] > 0:
        sys.exit(1)

if __name__ == "__main__":
    run_tests()