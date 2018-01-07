#!/usr/bin/env python

from enum import Enum

__all__ = ['BenchmarkCase', 'cases']


class BenchmarkCase(Enum):
    CLIENT2TEE = 'client2tee'
    KMOD2TEE = 'kernel2tee'

    def __str__(self):
        return str(self.value)


def benchmark_client2tee(timestamp_parser):
    print("Analyzing client2tee")
    raise NotImplementedError


def benchmark_kernelmode2tee(timestamp_parser):
    print("Analyzing kernel2tee")
    raise NotImplementedError


cases = {
         BenchmarkCase.CLIENT2TEE: benchmark_client2tee,
         BenchmarkCase.KMOD2TEE: benchmark_kernelmode2tee
         }
