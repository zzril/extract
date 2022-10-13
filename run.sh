#! /bin/sh -e

command="./extract"
outfile="examples/tmpfile"

make

for example in examples/*; do
	printf "\nExtracting bytes from \"$example\"...\n"
	$command -o 0x20 -l 0x10 "$example" > "$outfile" || true
	file "$outfile"
done


