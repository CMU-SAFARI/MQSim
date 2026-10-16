#!/usr/bin/env python3
"""Test MQSim's actual CMR matrix operations (baseline vs. local worktree).

Usage: python3 test_cmr_portable_regression.py MQSIM_REPO [BASELINE_REF]

Builds small UBSan-instrumented C++ harnesses from extracted source functions.
Checks patched boundary results and portable uint64_t against Python's
arbitrary-precision arithmetic for both CMR moduli. If BASELINE_REF is supplied,
also checks that its source triggers UBSan overflow in the same boundary cases.
Does not run an end-to-end SSD simulation.
"""
import pathlib
import random
import re
import subprocess
import sys
import tempfile

HEADER = "src/utils/CMRRandomGenerator.h"
MODULI = (4294967087, 4294944443)


def extract(source, name):
    match = re.search(r"\bstatic\s+void\s+" + re.escape(name) + r"\s*\(", source)
    if match is None:
        raise RuntimeError("Function not found: " + name)
    start = source.find("{", match.end())
    if start < 0:
        raise RuntimeError("Missing opening brace: " + name)
    depth = 0
    for pos in range(start, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():pos + 1]
    raise RuntimeError("Missing closing brace: " + name)


def cpp_header(source, names):
    return (
        "#include <cstdint>\n#include <iostream>\n"
        "class CMRProbe { public:\n"
        + "\n".join(extract(source, name) for name in names)
        + "\n};\n"
    )


def compile_cpp(directory, name, content):
    cpp = directory / (name + ".cpp")
    exe = directory / name
    cpp.write_text(content)
    command = ["g++", "-std=c++11", "-O0", "-g", "-fsanitize=undefined",
               "-fno-sanitize-recover=undefined", str(cpp), "-o", str(exe)]
    completed = subprocess.run(command, text=True, capture_output=True)
    if completed.returncode != 0:
        raise RuntimeError(f"Compilation failed [{name}]:\n{completed.stderr[:3000]}")
    return exe


def boundary_harness(source, method):
    helpers = ["v_copy", "mv_mul"] if method == "mv_mul" else ["m_copy", "mm_mul"]
    if method == "mv_mul":
        body = """
    int64_t a[3][3] = {{x,0,0},{0,0,0},{0,0,0}};
    int64_t u[3] = {x,0,0};
    int64_t v[3] = {};
    CMRProbe::mv_mul(a,u,v,m);
    std::cout << "RESULT=" << v[0] << "\\n";
    return v[0] == 1 ? 0 : 3;
"""
    else:
        body = """
    int64_t a[3][3] = {{x,0,0},{0,0,0},{0,0,0}};
    int64_t b[3][3] = {{x,0,0},{0,0,0},{0,0,0}};
    int64_t c[3][3] = {};
    CMRProbe::mm_mul(a,b,c,m);
    std::cout << "RESULT=" << c[0][0] << "\\n";
    return c[0][0] == 1 ? 0 : 3;
"""
    return cpp_header(source, helpers) + f"int main() {{\n const int64_t m={MODULI[0]}LL;\n const int64_t x=m-1;\n" + body + "\n}\n"


def oracle_harness(source):
    return cpp_header(source, ["v_copy", "m_copy", "mv_mul", "mm_mul"]) + r"""
int main() {
    int64_t m;
    while (std::cin >> m) {
        int64_t a[3][3], b[3][3], u[3], v[3]={}, c[3][3]={};
        for (int i=0; i<3; ++i) for (int j=0; j<3; ++j) std::cin >> a[i][j];
        for (int j=0; j<3; ++j) std::cin >> u[j];
        for (int i=0; i<3; ++i) for (int j=0; j<3; ++j) std::cin >> b[i][j];
        if (!std::cin) return 7;
        CMRProbe::mv_mul(a, u, v, m);
        CMRProbe::mm_mul(a, b, c, m);
        for (int i=0; i<3; ++i) std::cout << v[i] << ' ';
        for (int i=0; i<3; ++i) for (int j=0; j<3; ++j) std::cout << c[i][j] << ' ';
        std::cout << '\n';
    }
    return 0;
}
"""


def get_cases():
    rng = random.Random(20261016)
    for m in MODULI:
        for special in (0, 1, m // 2, m - 2, m - 1):
            yield m, [special]*9, [special]*3, [special]*9
        for _ in range(100):
            yield m, [rng.randrange(m) for _ in range(9)], \
                [rng.randrange(m) for _ in range(3)], \
                [rng.randrange(m) for _ in range(9)]


def expected(case):
    m, a, u, b = case
    v = [sum(a[3*i+j] * u[j] for j in range(3)) % m for i in range(3)]
    c = [sum(a[3*i+k] * b[3*k+j] for k in range(3)) % m
         for i in range(3) for j in range(3)]
    return v + c


def main():
    if len(sys.argv) not in (2, 3):
        print("Usage: python3 test_cmr_portable_regression.py MQSIM_REPO [BASELINE_REF]")
        return 2
    repo = pathlib.Path(sys.argv[1]).expanduser().resolve()
    baseline = sys.argv[2] if len(sys.argv) == 3 else None
    try:
        current = (repo / HEADER).read_text()
        if "__int128" in current:
            raise RuntimeError("Current source still contains __int128; expected portable fix")
        original = None
        if baseline is not None:
            original = subprocess.check_output(
                ["git", "-C", str(repo), "show", baseline + ":" + HEADER],
                text=True)
        with tempfile.TemporaryDirectory(prefix="mqsim_cmr_portable_") as tmp:
            directory = pathlib.Path(tmp)
            for name in ("mv_mul", "mm_mul"):
                good = compile_cpp(directory, "patched_" + name, boundary_harness(current, name))
                good_run = subprocess.run([str(good)], capture_output=True, text=True)
                ok = good_run.returncode == 0 and "RESULT=1" in good_run.stdout
                saw_overflow = None
                if original is not None:
                    bad = compile_cpp(directory, "original_" + name,
                                      boundary_harness(original, name))
                    bad_run = subprocess.run([str(bad)], capture_output=True, text=True)
                    saw_overflow = "signed integer overflow" in bad_run.stderr.lower()
                    print(f"{name}: baseline_ubsan_overflow={int(saw_overflow)} portable_pass={int(ok)}")
                else:
                    print(f"{name}: portable_pass={int(ok)}")
                if not ok or saw_overflow is False:
                    print("PATCHED STDOUT:", good_run.stdout[:1000])
                    print("PATCHED STDERR:", good_run.stderr[:1000])
                    if original is not None:
                        print("BASELINE STDERR:", bad_run.stderr[:1000])
                    return 1
            exe = compile_cpp(directory, "oracle", oracle_harness(current))
            cases = list(get_cases())
            inputs = "".join(" ".join(map(str, [m] + a + u + b)) + "\n"
                             for m, a, u, b in cases)
            result = subprocess.run([str(exe)], input=inputs, capture_output=True, text=True)
            if result.returncode != 0:
                print("ORACLE HARNESS FAILED:", result.stderr[:2000])
                return 1
            lines = result.stdout.splitlines()
            if len(lines) != len(cases):
                raise RuntimeError(f"Expected {len(cases)} oracle lines, got {len(lines)}")
            for idx, (line, case) in enumerate(zip(lines, cases)):
                values = list(map(int, line.split()))
                if values != expected(case):
                    print(f"NUMERIC_MISMATCH case={idx} modulus={case[0]}")
                    print("got:", values)
                    print("expected:", expected(case))
                    return 1
            print(f"PYTHON_BIGINT_ORACLE_PASS cases={len(cases)} moduli=2")
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as exc:
        print("CMR_TEST_ERROR:", exc)
        return 2
    print("CMR_PORTABLE_REGRESSION_PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
