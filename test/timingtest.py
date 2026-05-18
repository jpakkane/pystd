#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 Jussi Pakkanen

# Compiling a helloworld must never take more than a fraction of a second.

import os, sys, subprocess, shutil, json, time
from glob import glob

# GCC gets by in 0.1 seconds, but clang needs more.
MAX_RUNTIME = 0.15

def get_std(options):
    for o in options:
        if o['name'] == 'cpp_std':
            return o['value']
    sys.exit('C++ std value could not be determined.')

slow_headers = []

def test_header(compile_command_base, fname):
    global slow_headers
    compile_cmd = compile_command_base + [fname]

    fastest_time = 100
    num_tries = 5
    for i in range(num_tries):
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
    slow_headers.append((fname, fastest_time))

def get_header_list(source_root):
    headers = []
    for fname in glob(os.path.join(source_root, 'include/*2026*.hpp')):  # REMEMBER: update to match epoch.
        basename = os.path.basename(fname)
        headers.append(basename)
    assert len(headers) > 0
    return headers

def test_unixy():
    global slow_headers
    compilers = json.loads(open('meson-info/intro-compilers.json', 'r').read())
    options = json.loads(open('meson-info/intro-buildoptions.json', 'r').read())
    mi = json.loads(open('meson-info/meson-info.json', 'r').read())
    cpp = compilers['build']['cpp']['exelist']
    std_version = get_std(options)
    std_flags = [f'-std={std_version}'] if std_version else []

    builddir = mi['directories']['build']
    sourcedir = mi['directories']['source']
    build_to_src = os.path.relpath(sourcedir, builddir)

    fname = os.path.join(build_to_src, 'test/timingtest.cpp')
    fname_in = os.path.join(build_to_src, 'test/timingtest.cpp.in')
    compile_command_base = cpp + std_flags + ['-nostdinc++',
                                              '-Iinclude',
                                              '-I' + os.path.join(build_to_src, 'include'),
                                              '-c',
                                              '-o',
                                              '/dev/null']

    contents = open(fname_in, 'r').read()

    for header_name in get_header_list(sourcedir):
        open(fname, 'w').write(contents.replace('PYSTD_HEADER', header_name))
        test_header(compile_command_base, fname)
        os.unlink(fname)

    if slow_headers:
        print('Slow headers:\n')
        for fname, time in slow_headers:
            print(time, fname)
        sys.exit(1)

if __name__ == "__main__":
    if sys.platform == 'win32':
        sys.exit('Windows not supported yet.')
    else:
        test_unixy()
