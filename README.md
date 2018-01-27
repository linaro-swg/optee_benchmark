# OP-TEE Benchmark application
## Contents
1. [Introduction](#1-introduction)
2. [Implementation details](#2-implementation-details)
3. [Generating latency data](#3-generating-latency-data)
4. [Analyzing results](#4-analyzing-results)

## 1. Introduction

This README contains information about examples of usage
and implementation details of benchmark application.

For a generic description of benchmark feature, its components in OP-TEE OS
core, Linux TEE driver and libteec, please check [optee_benchmark.md].

---

## 2. Implementation details

Benchmark application consists of two parts:
- **Benchmark Client Application**, which is mainly used to generate latency
  data. It should be invoked on a destination SoC.
- **Benchmark analyze application**, which is used to analyze benchmark latency
  data. We recommend to use it on the same PC, where OP-TEE and related
  components were built, otherwise you won't receive proper translations of
  addresses to filenames and line numbers

## 2.1 Benchmark CA

## 2.2 Analyze application
`analyze.py` supports both Python 2 and 3. If you intend to use Python 2 or
any other version >=3.4, you should install in advance back-ported
`enum34` and `pathlib` packages using `pip` or `easy_install` package managers
(depends on your preferance).

```bash
$ pip2 install enum34
$ pip2 install pathlib
```

All logic for analyzing specific benchmark cases are stored in
`analyze/benchcases.py` file. If you want to add your own case, you have to
implement just one function which accepts as a param `TimestampParser` iterator,
which helps to access all parsed set of timestamps with auxilary data from the
timestamp dump file.

Each `Timestamp` includes such information:
- Counter value
- Subsystem, where timestamp was generated
- Core: identifies core id, where thread was running when timestamp had been
  generated.
- Address in subsystem, where timestamp was generated from.
- Source file and line for address: `Timestamp` instance can automatically
  resolve these values from symbol files, provided in `analyze/config.py`
- Payload (not standartized). It could contain such values, like:
    - Session ID: to distinguish one TEE session from another.

If you want to contribute patches, you	should check if they follow PEP8
style guide before creating any pull requests.

---

## 3. Generating latency data
Before running any benchmarks, OP-TEE OS and other compoments should be built
with `CFG_TEE_BENCHMARK` flag enabled. Check [optee_benchmark] for additional
details.

To collect benchmark data, you should invoke `benchmark`app and supply it with
a path regular CA you won't to benchmark as a param.

```bash
$ benchmark client_app [client_app params]
```

When CA finishes its execution, `benchmark` will create
`<client_app>.ts` timestamp data file by the same path, where original CA is
stored.

Check example:

```bash
$ benchmark /bin/xtest -l 15
```
After `xtest` successfully finishes running, `/bin/xtest.ts` will be created.

---

## 4. Analyzing results

Basically, you can receive all possible params for `analyze.py` just by
requesting internal help:

```bash
$ ./analyze.py -h
```

Example of analyzing client to tee benchmark case (using `-c` you specify a
benchmark case; `optee_example_hello_world.ts` is file containing latency data
generated in section 3):

```bash
$ ./analyze.py -c client2tee ../optee_example_hello_world.ts
```

[optee_benchmark]: https://github.com/OP-TEE/optee_os/blob/master/documentation/benchmark.md
