extract
=======

Extract bytes from a file.

General:
--------

A small command-line utility to extract/copy parts of a file, with a given  
start/offset and length.  
Might be useful for some forensics/steganography CTF challenges.

Usage:
------

`./bin/extract -i infile [-o outfile] [-s seek] [-l length]`  
When `length` is 0 or omitted, the file will be copied from `seek` to end.  
If `outfile` is not specified, output will be written to a newly created file  
`outfile` in the current working directory.

Build:
------

* `make` will compile everything and put the binary in `./bin/extract`.
* `make clean` will clean everything for a future clean build.

Test:
-----

* The script `run.sh` will run some tests on the examples in `./examples`.

Contribute:
-----------

If you have any suggestions, notice any bugs etc., feel free to open an issue  
about it.  
Direct contributions via pull requests are also welcome.


