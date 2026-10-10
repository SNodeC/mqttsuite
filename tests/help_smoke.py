"""SNode.C returns 2 when bootstrap prints help instead of starting the loop."""
import subprocess
import sys

result = subprocess.run([sys.argv[1], "--help"], capture_output=True, text=True, timeout=5)
print(result.stdout, end="")
print(result.stderr, end="", file=sys.stderr)
assert result.returncode == 2, f"unexpected help exit code: {result.returncode}"
assert "Usage:" in result.stdout and "powered by SNode.C" in result.stdout
assert "[FileError]" not in result.stdout and "[ParseError]" not in result.stdout
