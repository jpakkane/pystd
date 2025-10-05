#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 Jussi Pakkanen

import os, sys
from typing import List

counts: dict[str, int] = {}

with open(sys.argv[1], 'r') as f:
    line: str = ''
    for line in f:
        word: str = ''
        for word in line.split():
            try:
                counts[word] += 1
            except KeyError:
                counts[word] = 1

stats: List[tuple[int, str]] = []

for word, count in counts.items():
    stats.append((count, word))

stats.sort()
stats.reverse()
for count, word in stats:
    print(count, word)
