/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <fcntl.h>
#include <libgen.h>
#include <linux/limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "benchmark_aux.h"
#include "common.h"

/* Misc auxilary functions */
void tee_errx(const char *msg, TEEC_Result res)
{
	ERROR_EXIT("%s: 0x%08x\n", msg, res);
}

void tee_check_res(TEEC_Result res, const char *errmsg)
{
	if (res != TEEC_SUCCESS)
		tee_errx(errmsg, res);
}

const char *bench_str_src(uint64_t source)
{
	switch (source) {
	case TEE_BENCH_CORE:
		return "core";
	case TEE_BENCH_KMOD:
		return "kmodule";
	case TEE_BENCH_CLIENT:
		return "libteec";
	case TEE_BENCH_UTEE:
		return "libutee";
	default:
		return "-";
	}
}

void print_line(void)
{
		int n = 115;

		while (n-- > 0)
			printf("=");
		printf("\n");
}

void alloc_argv(int argc, char *argv[], char **new_argv[])
{
	char *res, *base;
	char path[PATH_MAX];
	char **testapp_argv;
	int i;

	res = realpath(argv[1], path);
	if (!res)
		_exit(EXIT_FAILURE);

	*new_argv = malloc(argc * sizeof(void *));
	if (!(*new_argv))
		_exit(EXIT_FAILURE);

	testapp_argv = *new_argv;
	testapp_argv[argc-1] = NULL;
	base = basename(path);

	testapp_argv[0] = malloc(strlen(base) + 1);
	if (!testapp_argv[0])
		_exit(EXIT_FAILURE);

	memcpy(testapp_argv[0], base, strlen(base) + 1);

	for (i = 2; i < argc; i++) {
		size_t length = strlen(argv[i]) + 1;

		testapp_argv[i - 1] = malloc(length);
		if (!testapp_argv[i - 1])
			_exit(EXIT_FAILURE);

		memcpy(testapp_argv[i-1], argv[i], length);
	}
}

void dealloc_argv(int new_argc, char **new_argv)
{
	int i;
	for (i = 0; i < new_argc; ++i)
		free(new_argv[i]);

	free(new_argv);
}

uint32_t get_cores(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

void *mmap_paddr(intptr_t paddr, uint64_t size)
{
	int devmem;
	off_t offset = 0;
	off_t page_addr;
	intptr_t *hw_addr = (intptr_t *)paddr;

	devmem = open("/dev/mem", O_RDWR);
	if (!devmem)
		return NULL;

	offset = (off_t)hw_addr % getpagesize();
	page_addr = (off_t)(hw_addr - offset);

	hw_addr = (intptr_t *)mmap(0, size, PROT_READ|PROT_WRITE,
					MAP_SHARED, devmem, page_addr);
	if (hw_addr == MAP_FAILED) {
		close(devmem);
		return NULL;
	}

	close(devmem);
	return (hw_addr + offset);
}
