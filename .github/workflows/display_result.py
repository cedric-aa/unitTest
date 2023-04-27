import json
import sys

def main():
    if len(sys.argv) != 2:
        print("Usage: python display_results.py path/to/twister.json")
        sys.exit(1)

    json_file = sys.argv[1]
    with open(json_file, 'r') as f:
        data = json.load(f)

    print("Test Results Summary\n")
    print("Environment:")
    for key, value in data["environment"].items():
        print(f"  {key}: {value}")

    print("\nTestsuites:")
    for testsuite in data["testsuites"]:
        print(f"\n  {testsuite['name']}:")
        print(f"    Status: {testsuite['status']}")
        print(f"    Execution time: {testsuite['execution_time']}")

        print("    Testcases:")
        for testcase in testsuite["testcases"]:
            print(f"      - {testcase['identifier']}:")
            print(f"          Status: {testcase['status']}")
            print(f"          Execution time: {testcase['execution_time']}")

if __name__ == "__main__":
    main()
