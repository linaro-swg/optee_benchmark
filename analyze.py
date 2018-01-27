#!/usr/bin/env python
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2017, Linaro Limited
#

import sys

# path to all benchmark modules
sys.path.append("analyze/")

import argparse
import config

from benchcases import *
from pathlib import Path
from timestamp import *

# handle params
arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("filename", help="File with timestamps")
arg_parser.add_argument("-c", "--case", type=BenchmarkCase,
                        choices=list(BenchmarkCase),
                        help="Specify the benchmark case you want to analyze")
args = arg_parser.parse_args()

try:
    if not (args.filename and Path(args.filename).is_file()):
        raise RuntimeError("Cannot find file: {}.".format(args.filename))

    ts_parser = TimestampParser(Path(args.filename))

# run benchmark case
    if args.case:
        cases[args.case](ts_parser)
    else:
        cases[0](ts_parser)
except Exception as inst:
    print("Error occured: " + str(inst))
