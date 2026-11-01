#!/usr/bin/env bash

# Script exits on any failing command (-e),
# on use of unset variables (-u),
# and on failures inside pipelines (-o pipefail).
set -e -u -o pipefail

# This script lives in the project root directory.
base_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

modules_dir="$base_dir/data"
modules_dir=$(cd "$modules_dir" && pwd)
echo Take uncompressed JSON modules from "$modules_dir"
echo "Processing $(find "$modules_dir" -name '*.json' | wc -l | tr -d ' ') modules"

# The bibledit.org staging directory where the compressed modules go.
staging_dir=$(realpath ~/dev/website/bibledit.org/site/html/modules)
echo Stage into "$staging_dir"

echo Compressing them all
for file in "$modules_dir"/*.json; do
    output_file="$staging_dir/$(basename "$file").gz"
    echo Compress "$file" to "$output_file"
    # Do not store the original filename and timestamp in the gzip header
    gzip -9 --no-name --stdout "$file" > "$output_file"
done

echo "Done"
echo Use the upload script for all sites to upload this to bibledit.org