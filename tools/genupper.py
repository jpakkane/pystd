#!/usr/bin/env python3

import sys

print('Multiple chars.')

multichars = []

for i in range(0x110000):
    string = chr(i)
    assert(len(string) == 1)
    upstring = string.upper()
    if len(upstring) != 1:
        multichars.append((i, upstring))

print('// clang-format off')
print('#define NUM_UPPERCASING_MULTICHAR_ENTRIES', len(multichars))
print('const UnicodeConversionMultiChar uppercasing_multi[NUM_UPPERCASING_MULTICHAR_ENTRIES] = {')
for i, upstring in multichars:
    definition = f'    {{{i}, {{'
    entry = [ord(c) for c in upstring]
    while len(entry) < 3:
        entry.append(0)
    definition += ', '.join(['%d' % c for c in entry])
    definition += '}},'
    print(definition)
print('};')
print('// clang-format on')

print('\nDifferences')
print('// clang-format off')
diffs = []
for i in range(128, 0x110000):
    string = chr(i)
    assert(len(string) == 1)
    upstring = string.upper()
    if len(upstring) == 1:
        if upstring != string:
            diffs.append((i, ord(upstring)))

print('#define NUM_UPPERCASING_SINGLE_ENTRIES', len(diffs))
print('const UnicodeConversionSingleChar uppercasing_single[NUM_UPPERCASING_SINGLE_ENTRIES] = {')
for from_cp, to_cp in diffs:
    print(f'    {{{from_cp}, {to_cp}}},')
print('};')
print('// clang-format on')

