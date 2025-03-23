#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 Jussi Pakkanen

import os, sys

counts = {}

with open(sys.argv[1], 'r') as f:
    for line in f:
        for word in line.split():
            try:
                counts[word] += 1
            except KeyError:
                counts[word] = 1

stats = []
for word, count in counts.items():
    stats.append((count, word))

stats.sort()
stats.reverse()
for count, word in stats:
    print(count, word)
