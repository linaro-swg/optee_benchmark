#!/usr/bin/env python
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2017, Linaro Limited
#

from enum import Enum
import sys

__all__ = ['BenchmarkCase', 'cases']


if __name__ == '__main__':
    sys.exit("This source file should be used only as a module")


def whoami():
        return sys._getframe(1).f_code.co_name


class BenchmarkCase(Enum):
    CLIENT2TEE = 'client2tee'
    KMOD2TEE = 'kernel2tee'

    def __str__(self):
        return str(self.value)


def benchmark_client2tee(timestamp_parser):
    raise NotImplementedError(
        "{} is not implemented".format(whoami()))


def benchmark_kernelmode2tee(timestamp_parser):
    raise NotImplementedError(
        "{} is not implemented".format(whoami()))


cases = {
         BenchmarkCase.CLIENT2TEE: benchmark_client2tee,
         BenchmarkCase.KMOD2TEE: benchmark_kernelmode2tee
         }
