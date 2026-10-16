#!/usr/bin/env python3

"""
Regression test for Flash_Block_Manager_Base::Is_page_valid.

Extracts the production function from MQSim and checks bitmap
word boundaries in an isolated C++ harness under UBSan.

Requires Python 3 and GCC with UBSan support.
"""

from pathlib import Path
import subprocess
import tempfile
import sys
import re


def extract_function(source):
    pattern = r"\bbool\s+Flash_Block_Manager_Base::Is_page_valid\s*\("

    match = re.search(pattern, source)

    if match is None:
        raise RuntimeError("Is_page_valid not found")

    start = match.start()
    opening = source.find("{", match.end())

    if opening < 0:
        raise RuntimeError("Function body not found")

    depth = 0

    for i in range(opening, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1

            if depth == 0:
                return source[start:i + 1]

    raise RuntimeError("Unbalanced function body")


def main():
    root = (
        Path(sys.argv[1]).resolve()
        if len(sys.argv) > 1
        else Path(__file__).resolve().parents[1]
    )

    source_file = (
        root / "src/ssd/Flash_Block_Manager_Base.cpp"
    )

    function = extract_function(
        source_file.read_text()
    )

    test_source = r'''
#include <cstdint>
#include <iostream>

using flash_page_ID_type = unsigned int;

namespace SSD_Components {

struct Block_Pool_Slot_Type {
    uint64_t* Invalid_page_bitmap;
};

class Flash_Block_Manager_Base {
public:
    bool Is_page_valid(
        Block_Pool_Slot_Type* block,
        flash_page_ID_type page_id
    );
};

''' + function + r'''

}

int main()
{
    using namespace SSD_Components;

    uint64_t bitmap[4] = {0, 0, 0, 0};

    Block_Pool_Slot_Type block{bitmap};
    Flash_Block_Manager_Base manager;

    const unsigned int pages[] = {
        0, 1, 62, 63,
        64, 65, 126, 127,
        128, 129, 254, 255
    };

    for (unsigned int page : pages) {

        for (auto& word : bitmap) {
            word = 0;
        }

        // Initially valid
        if (!manager.Is_page_valid(&block, page)) {
            std::cerr << "FAIL_VALID=" << page << '\n';
            return 10;
        }

        // Mark selected page invalid
        bitmap[page / 64] =
            uint64_t(1) << (page % 64);

        if (manager.Is_page_valid(&block, page)) {
            std::cerr << "FAIL_INVALID=" << page << '\n';
            return 11;
        }

        std::cout << "PASS_PAGE=" << page << '\n';
    }

    std::cout << "ALL_BITMAP_TESTS_PASS\n";

    return 0;
}
'''

    with tempfile.TemporaryDirectory() as temporary:

        temporary = Path(temporary)

        cpp = temporary / "bitmap_test.cpp"
        exe = temporary / "bitmap_test"

        cpp.write_text(test_source)

        build = subprocess.run(
            [
                "g++",
                "-std=c++11",
                "-O0",
                "-g",
                "-fsanitize=undefined",
                "-fno-sanitize-recover=all",
                str(cpp),
                "-o",
                str(exe),
            ],
            capture_output=True,
            text=True,
        )

        if build.returncode:
            print(build.stdout)
            print(build.stderr, file=sys.stderr)
            return build.returncode

        run = subprocess.run(
            [str(exe)],
            capture_output=True,
            text=True,
        )

        print(run.stdout, end="")

        if run.stderr:
            print(run.stderr, file=sys.stderr, end="")

        return run.returncode


if __name__ == "__main__":
    sys.exit(main())
