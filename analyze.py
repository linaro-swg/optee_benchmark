#!/usr/bin/env python

import argparse
from pathlib import Path
import sys

# path to all benchmark modules
sys.path.append("analyze/")
if sys.version_info[0] < 3:
    raise "Python 3 should be used to run this script"

import config
from timestamp import *
from benchcases import *

# handle params
arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("filename", help="File with timestamps")
arg_parser.add_argument("-c", "--case", type=BenchmarkCase,
                        choices=list(BenchmarkCase),
                        help="Specify the benchmark case you want to analyze")
args = arg_parser.parse_args()

try:
    if not (args.filename and Path(args.filename).is_file()):
        raise RuntimeError("Wrong path to timestamp dump file")

    ts_parser = TimestampParser(Path(args.filename))

    if args.case:
        cases[args.case](ts_parser)
    else:
        bench_case = 0
except Exception as inst:
    print("Error occured: " + str(inst))
