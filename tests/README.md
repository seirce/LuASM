# Translation tests

After building, run `ctest --test-dir build -C Release --output-on-failure`.

The suite translates a beam-block table through both command-line tools and compares the output bytes, including indentation, comments, blank lines and line endings. It creates temporary nested libraries in the build tree, checks target-specific include syntax, and verifies that a failed translation preserves the last generated patch.
