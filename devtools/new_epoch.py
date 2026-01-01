#!/usr/bin/env python3

import pathlib, datetime

def copy_files(subdir, old_year, new_year):
    for ifile in pathlib.Path(subdir).glob(f'*{old_year}*'):
        ofile = pathlib.Path(str(ifile).replace(old_year, new_year))
        contents = ifile.read_text()
        contents = contents.replace(f'pystd{old_year}', f'pystd{new_year}')
        contents = contents.replace(f'PYSTD{old_year}', f'PYSTD{new_year}')
        ofile.write_text(contents)

def create_epoch(old_year, new_year):
    copy_files('include', old_year, new_year)
    copy_files('src', old_year, new_year)
    copy_files('test', old_year, new_year)


if __name__ == '__main__':
    today = datetime.date.today()
    old_year = str(today.year - 1)
    new_year = str(today.year)
    create_epoch(old_year, new_year)
