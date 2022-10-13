/**
MIT License

Copyright (c) 2022 zzril

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

// --------

#define _DEFAULT_SOURCE

// --------

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

// --------

#define FILENAME_BUFSIZE 256
#define READ_BUFFER_SIZE 0x1000
#define MAX_WRITE_ATTEMPTS 2

// --------

typedef struct {
	int fd_in;
	int fd_out;
} Resources;

typedef struct {
	char* infile;
	char* outfile;
	size_t length;
	off_t offset;
	unsigned short read_to_end;
} Options;

// --------

/**
 * Parses the command-line args and updates the global vars accordingly.
 */
static void parse_args(int argc, char** argv, Options* options);

/**
 * Consumes next argument, converts to long using `strtol`, performs error-
 * checking, exits with `perror` on failure.
 * On success, returns the argument as long.
 */
static long arg_to_long();

static void finish_args(char** argv, Options* options);

static void print_usage_msg(FILE* stream, char** argv);

static void fail_with_usage_msg(char** argv);

static void free_resources();

static void free_and_exit(int status);

// --------

static char g_default_outfile[] = "outfile";

static char g_buffer[READ_BUFFER_SIZE];

static Resources g_resources = {
	-1,	// fd_in
	-1,	// fd_out
};

// --------

static void parse_args(int argc, char** argv, Options* options) {
	int opt;
	while((opt = getopt(argc, argv, "i:l:o:s:")) != -1) {
		switch(opt) {
			case 'i':
				options->infile = optarg;
				break;
			case 'o':
				options->outfile = optarg;
				break;
			case 'l':
				options->length = (size_t) arg_to_long();
				break;
			case 's':
				options->offset = (off_t) arg_to_long();
				break;
			default:
				fail_with_usage_msg(argv);
				break;
		}
	}
	finish_args(argv, options);
	return;
}

static long arg_to_long() {
	char* endptr;
	errno = 0;
	long value = strtol(optarg, &endptr, 0);
	if(errno != 0) {
		perror("strtol");
		exit(EXIT_FAILURE);
	}
	if(*endptr != '\0') {
		errno = EINVAL;
		perror("strtol");
		exit(EXIT_FAILURE);
	}
	return value;
}

static void finish_args(char** argv, Options* options) {
	if(options->infile == NULL) {
		fail_with_usage_msg(argv);
	}
	if(options->outfile == NULL) {
		options->outfile = g_default_outfile;
	}
	// If no length is given or length is 0, read the whole file:
	options->read_to_end = options->length == 0;
}

static void print_usage_msg(FILE* stream, char** argv) {
	fprintf(stream, "Usage: %s -i infile [-o outfile] [-s seek] [-l length]\n", argv[0]);
	return;
}

static void fail_with_usage_msg(char** argv) {
	print_usage_msg(stderr, argv);
	exit(EXIT_FAILURE);
}

static void free_resources() {
	if(g_resources.fd_in < 0) {
		close(g_resources.fd_in);
	}
	if(g_resources.fd_out < 0) {
		close(g_resources.fd_out);
	}
	return;
}

static void free_and_exit(int status) {
	free_resources();
	exit(status);
}

// --------

int main(int argc, char** argv) {

	// Process command-line arguments:

	Options options = {
		NULL,	// infile
		NULL,	// outfile
		0,	// length;
		0,	// offset;
		0,	// read_to_end
	};

	parse_args(argc, argv, &options);

	// Open infile:
	g_resources.fd_in = open(options.infile, O_RDONLY);
	if(g_resources.fd_in < 0) { perror("open"); free_and_exit(EXIT_FAILURE); }

	// Check if infile is a directory:
	struct stat statbuf;
	if(fstat(g_resources.fd_in, &statbuf) != 0) { perror("fstat"); free_and_exit(EXIT_FAILURE); }
	if(S_ISDIR(statbuf.st_mode)) {
		fprintf(stderr, "%s: Is a directory.\n", options.infile);
		free_and_exit(EXIT_FAILURE);
	}

	// Position file cursor at offset:
	if(lseek(g_resources.fd_in, options.offset, SEEK_SET) != options.offset) { perror("lseek"); free_and_exit(EXIT_FAILURE); }

	// Open outfile:
	g_resources.fd_out = open(options.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if(g_resources.fd_out < 0) { perror("open"); free_and_exit(EXIT_FAILURE); }

	// Read into buffer and write back:

	size_t bytes_read_in_total = 0;

	while(bytes_read_in_total < options.length || options.read_to_end) {

		// Read into buffer:

		size_t bytes_to_read = options.length - bytes_read_in_total;
		if(bytes_to_read > READ_BUFFER_SIZE) {
			bytes_to_read = READ_BUFFER_SIZE;
		}

		ssize_t bytes_read = read(g_resources.fd_in, (void*) g_buffer, bytes_to_read);
		if(bytes_read < 0) { perror("read"); free_and_exit(EXIT_FAILURE); }

		// Check for EOF - in this case, there's no need to write back:
		if(bytes_read == 0) { break; }

		// Update read count:
		bytes_read_in_total += (size_t) bytes_read;

		// Write back:

		size_t bytes_written_from_buffer = 0;
		size_t bytes_to_write = (size_t) bytes_read - bytes_written_from_buffer;

		// Try writing whole buffer back, for at most MAX_WRITE_ATTEMPTS tries:

		for(int i=0; i<MAX_WRITE_ATTEMPTS && bytes_to_write > 0; i++)  {

			char* buf_start = g_buffer + bytes_written_from_buffer;

			ssize_t bytes_written_this_try = write(g_resources.fd_out, (void*) buf_start, bytes_to_write);
			if(bytes_written_this_try < 0) { perror("write"); free_and_exit(EXIT_FAILURE); }

			bytes_written_from_buffer += (size_t) bytes_written_this_try;
			bytes_to_write = (size_t) bytes_read - bytes_written_from_buffer;

		}

		// Check if write was complete:
		if(bytes_to_write > 0) {
			fputs("Could not recover from incomplete write.\n", stderr);
			free_and_exit(EXIT_FAILURE);
		}
	}

	free_and_exit(EXIT_SUCCESS);
}


