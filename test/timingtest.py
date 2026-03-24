#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 Jussi Pakkanen

# Compiling a helloworld must never take more than a fraction of a second.

import os, sys, subprocess, shutil, json, time

MAX_RUNTIME = 0.1

def get_std(options):
    for o in options:
        if o['name'] == 'cpp_std':
            return o['value']
    sys.exit('C++ std value could not be determined.')

def test_unixy():
    compilers = json.loads(open('meson-info/intro-compilers.json', 'r').read())
    options = json.loads(open('meson-info/intro-buildoptions.json', 'r').read())
    mi = json.loads(open('meson-info/meson-info.json', 'r').read())
    cpp = compilers['build']['cpp']['exelist']
    std_version = get_std(options)
    std_flags = [f'-std={std_version}'] if std_version else []

    builddir = mi['directories']['build']
    sourcedir = mi['directories']['source']
    build_to_src = os.path.relpath(sourcedir, builddir)

    fastest_time = 100
    num_tries = 5
    for i in range(num_tries):
        compile_cmd = cpp + std_flags + ['-Iinclude',
                                         '-I' + os.path.join(build_to_src, 'include'),
                                         '-c',
                                         '-o',
                                         '/dev/null',
                                         os.path.join(build_to_src, 'test/timingtest.cpp')]
        starttime = time.time()
        pc = subprocess.run(compile_cmd)
        endtime = time.time()
        if pc.returncode != 0:
            sys.exit(pc.returncode)
        runtime = endtime - starttime
        if runtime < fastest_time:
            fastest_time = runtime
        if fastest_time < MAX_RUNTIME:
            return
    sys.exit(f'Build time test took too long: {fastest_time}.')


if __name__ == "__main__":
    if sys.platform == 'win32':
        sys.exit('Windows not supported yet.')
    else:
        test_unixy()
