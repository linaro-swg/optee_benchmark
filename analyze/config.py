#!/usr/bin/env python
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2018, Linaro Limited
#

import sys
from pathlib import Path
from timestamp import Subsystem

if __name__ == '__main__':
    sys.exit("This source file should be used only as a module")

subsystem_paths = {Subsystem.LIBTEEC:
                   Path('../optee_client/out/libteec/libteec.so.1.0'),
                   Subsystem.LINUXMOD:
                   Path('../linux/vmlinux'),
                   Subsystem.OPTEECORE:
                   Path('../optee_os/out/arm/core/tee.elf')}

addr2line = Path('../toolchains/aarch32/bin/arm-linux-gnueabihf-addr2line')
