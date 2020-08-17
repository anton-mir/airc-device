#!/bin/bash

CSCOPE_DIR="$PWD/cscope"

if [ ! -d "$CSCOPE_DIR" ]; then
	mkdir "$CSCOPE_DIR"
fi

echo "Finding files ..."
find "$PWD" \
    -name '*.[ch]' \
    -o -name '*.S' > "$CSCOPE_DIR/cscope.files"

echo "Adding files to cscope db: $PWD/cscope.db ..."
cscope -b -i "$CSCOPE_DIR/cscope.files"

export CSCOPE_DB="$PWD/cscope.out"
echo "Exported CSCOPE_DB to: '$CSCOPE_DB'"

echo "Generating ctags tags ..."
ctags -R
echo "Done"

