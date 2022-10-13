#! /bin/sh -e

command="./bin/extract"
outfile="./examples/out/tmpfile"

make

for example in ./examples/*; do
	if [ ! -d "$example" ]; then
		printf "\nExtracting bytes from \"$example\"...\n"
		$command -s 0x20 -l 0x10 -i "$example" -o "$outfile" && file "$outfile"
	fi
done


