#!/usr/bin/env python
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2017, Linaro Limited
#

import config
import io
from pathlib import Path
import subprocess
import sys
import yaml

from enum import Enum

__all__ = ["Timestamp", "Subsystem", "TimestampParser"]


class Subsystem(Enum):
    LIBTEEC = 1
    LINUXMOD = 2
    OPTEECORE = 3


class Addr2Line(object):
    def __init__(self, binary):
        if not (isinstance(config.addr2line, Path) and
                config.addr2line.is_file()):
            raise SystemError('addr2line binary was not found')
        if not (isinstance(binary, Path) and binary.is_file()):
            raise ValueError(str(binary) + " was not found")

        self.process = subprocess.Popen([str(config.addr2line),
                                        "-e", str(binary)],
                                        stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE)
        # and let's use buffered input
        self.stdin_wrapper = io.TextIOWrapper(self.process.stdin,
                                              'utf-8')

    def __del__(self):
        if self.process:
            self.process.terminate()

    def lookup(self, addr):
        try:
            print(hex(addr))
            self.stdin_wrapper.write(hex(addr) + '\n')
            self.stdin_wrapper.flush()
            info = self.process.stdout.readline().decode("utf-8")
            print(info)
        except IOError:
            raise RuntimeError("Can't communicate with addr2line")
        finally:
            ret = self.process.poll()
            if ret:
                raise RuntimeError(
                    "addr2line terminated unexpectedly")

        (file, line) = info.rsplit(":", 1)

        if file == "??":
            raise RuntimeError("Illegal binary file")
        if line == "0":
            raise RuntimeError("Illegal line")

        return (file, int(line))


class Timestamp(object):
    """Used to store a timestamp

    Able to parse address of binary file of subsystem and provide
    actual source file and line
    """
    _symbol_parsers = {}

    def __init__(self, core, counter, subsystem, address, payload):
        self.core = core
        self.counter = counter
        if subsystem is not Subsystem:
            raise TypeError("Wrong subsystem type")

        self.subsystem = subsystem
        self.address = long(address)
        self.payload = payload

        self._parse_addr()

    def _parse_addr(self):
        if self.address and self.subsystem:
            if self.subsystem not in self.__symbol_parsers:
                self._symbol_parsers[self.subsystem, Addr2Line(
                    config.subsystem_paths[self.subsystem])]

            (self.filename, self.line) = \
                self._symbol_parsers[self.subsystem].lookup(self.address)

        else:
            raise ValueError('Timestamp address/subsystem isn\'t initialized')

    def _parse_payload(self):
        raise NotImplementedError


class TimestampParser(object):
    def __init__(self, yaml_path):
        if not (isinstance(yaml_path, Path) and
                yaml_path.is_file()):
            raise RuntimeError("Cannot find file: {}.".format(str(yaml_path)))

        self.i = 0
        self.yaml_path = yaml_path
        self.doc = yaml.load(open(str(yaml_path), 'r'))
        self.n = len(self.doc["timestamps"])

    def __iter__(self):
        return self

    def __next__(self):
        return self.next()

    def next(self):
        if self.i >= self.n:
            raise StopIteration()

        print(self.doc["timestamps"][self.i])
        self.i += 1


if __name__ == '__main__':
    sys.exit("This source should be used only as a module")
