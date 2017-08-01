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

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "benchmark_aux.h"

#define STAT_AMOUNT 5
#define TSFILE_NAME_SUFFIX			".ts"

#define RING_SUCCESS	0
#define RING_BADPARM	-1
#define RING_NODATA		-2

static const TEEC_UUID pta_benchmark_uuid = PTA_BENCHMARK_UUID;
struct tee_ts_global *bench_ts_global;
static bool is_running;

static TEEC_SharedMemory ts_buf_shm = {
		.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT
};

static TEEC_Context ctx;
static TEEC_Session sess;

static void open_bench_pta(void)
{
	TEEC_Result res;
	uint32_t err_origin;

	res = TEEC_InitializeContext(NULL, &ctx);
	tee_check_res(res, "TEEC_InitializeContext");

	res = TEEC_OpenSession(&ctx, &sess, &pta_benchmark_uuid,
			TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	tee_check_res(res, "TEEC_OpenSession");
}

static void close_bench_pta(void)
{
	/* release benchmark timestamp shm */
	TEEC_ReleaseSharedMemory(&ts_buf_shm);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
}

static void init_ts_global(void *ts_global, uint32_t cores)
{
	unsigned int i;
	struct tee_ts_cpu_buf *cpu_buf;

	/* init global timestamp buffer */
	bench_ts_global = (struct tee_ts_global *)ts_global;
	bench_ts_global->cores = cores;

	/* init per-cpu timestamp buffers */
	for (i = 0; i < cores; i++) {
		cpu_buf = &bench_ts_global->cpu_buf[i];
		memset(cpu_buf, 0, sizeof(struct tee_ts_cpu_buf));
	}
}

static void register_bench_buf(uint32_t cores)
{
	TEEC_Result res;
	TEEC_Operation op = { 0 };
	uint32_t ret_orig;

	ts_buf_shm.size = sizeof(struct tee_ts_global) +
			sizeof(struct tee_ts_cpu_buf) * cores;

	/* allocate global timestamp buffer */
	res = TEEC_AllocateSharedMemory(&ctx, &ts_buf_shm);
	tee_check_res(res, "TEEC_AllocateSharedMemory");

	init_ts_global(ts_buf_shm.buffer, cores);

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INOUT,
			TEEC_NONE, TEEC_NONE, TEEC_NONE);
	op.params[0].memref.parent = &ts_buf_shm;

	TEEC_InvokeCommand(&sess, BENCHMARK_CMD_REGISTER_MEMREF,
					&op, &ret_orig);
	tee_check_res(res, "TEEC_InvokeCommand");
}

static void unregister_bench(void)
{
	TEEC_Result res;
	TEEC_Operation op = { 0 };
	uint32_t ret_orig;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
					TEEC_NONE, TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&sess, BENCHMARK_CMD_UNREGISTER,
					&op, &ret_orig);
	tee_check_res(res, "TEEC_InvokeCommand");
}

static void usage(char *progname)
{
	fprintf(stderr, "Call latency benchmark tool for OP-TEE\n\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s -h\n", progname);
	fprintf(stderr, "  %s host_app [host_app_args]\n", progname);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -h              Print this help and exit\n");
	fprintf(stderr, "  host_app        Path to host app to benchmark\n");
	fprintf(stderr, "  host_app_args   Original host app args\n");
}

static int timestamp_pop(struct tee_ts_cpu_buf *cpu_buf,
						struct tee_time_st *ts)
{
	uint64_t ts_tail;

	if (!cpu_buf && !ts)
		return RING_BADPARM;

	if (cpu_buf->tail >= cpu_buf->head)
		return RING_NODATA;

	ts_tail = cpu_buf->tail++;
	*ts = cpu_buf->stamps[ts_tail & TEE_BENCH_MAX_MASK];

	return 0;
}

static void *ts_consumer(void *arg)
{
	unsigned int i;
	int ret;
	bool ts_received = false;
	uint32_t cores;
	struct tee_time_st ts_data;
	FILE *ts_file;
	char *tsfile_path;

	tsfile_path = arg;
	if (!tsfile_path)
		goto exit;

	cores = get_cores();
	if (!cores)
		goto exit;

	ts_file = fopen(tsfile_path, "w");
	if (!ts_file)
		goto exit;

	while (is_running) {
		ts_received = false;
		for (i = 0; i < cores; i++) {
			ret = timestamp_pop(&bench_ts_global->cpu_buf[i],
						&ts_data);
			if (!ret) {
				ts_received = true;
				fprintf(ts_file, "%u\t%lld\t0x%"
						PRIx64 "\t%s\n",
						i, ts_data.cnt, ts_data.addr,
						bench_str_src(ts_data.src));
			}
		}

		if (!ts_received) {
			if (is_running)
				sched_yield();
			else
				goto file_close;
		}
	}

file_close:
	fclose(ts_file);
exit:
	return NULL;
}

int main(int argc, char *argv[])
{
	int i;
	int status;
	pid_t pid;
	char testapp_path[PATH_MAX];
	char **testapp_argv;
	char *res;
	char *tsfile_path;
	uint32_t cores;
	pthread_t consumer_thread;

	if (argc == 1) {
		usage(argv[0]);
		return 0;
	}

	/* Parse command line */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			usage(argv[0]);
			return 0;
		}
	}

	printf("1. Opening Benchmark Static TA...\n");
	open_bench_pta();

	cores = get_cores();
	if (!cores)
		tee_errx("Receiving amount of active cores failed",
					EXIT_FAILURE);

	printf("2. Allocating per-core buffers, cores detected = %d\n",
					cores);
	register_bench_buf(cores);

	res = realpath(argv[1], testapp_path);
	if (!res)
		tee_errx("Failed to get realpath", EXIT_FAILURE);

	alloc_argv(argc, argv, &testapp_argv);

	printf("3. Starting origin host app %s ...\n", testapp_path);

	/* fork/exec here */
	pid = fork();

	if (pid == -1) {
		tee_errx("fork() failed", EXIT_FAILURE);
	} else if (pid > 0) {
		is_running = 1;

		tsfile_path = malloc(strlen(testapp_path) +
					strlen(TSFILE_NAME_SUFFIX) + 1);
		if (!tsfile_path)
			return 1;

		tsfile_path[0] = '\0';
		strcat(tsfile_path, testapp_path);
		strcat(tsfile_path, TSFILE_NAME_SUFFIX);

		printf("Dumping timestamps to %s ...\n", tsfile_path);
		print_line();

		if (pthread_create(&consumer_thread, NULL,
				ts_consumer, tsfile_path)) {
			fprintf(stderr, "Error creating ts consumer thread\n");
			return 1;
		}
		/* wait for child app exits */
		waitpid(pid, &status, 0);
		is_running = 0;

		/* wait for our consumer thread terminate */
		if (pthread_join(consumer_thread, NULL)) {
			fprintf(stderr, "Error joining thread\n");
			return 2;
		}
	}
	else {
		execvp(testapp_path, testapp_argv);
		tee_errx("execve() failed", EXIT_FAILURE);
	}

	printf("4. Done benchmark\n");

	dealloc_argv(argc-1, testapp_argv);
	unregister_bench();
	close_bench_pta();
	return 0;
}
