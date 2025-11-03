#!/usr/bin/env python3
#
# Prints the GitHub download counters for all releases of OpenSim Creator
# to the standard output.

import datetime
import itertools
import functools
import requests  # must be installed via pip: `pip install requests`
import re

class Row:
    def __init__(self, name, version, created_at, downloads):
        self.name = name
        self.version = version
        self.created_at = created_at
        self.downloads = downloads


# these counters come from OSMV, which covers roughly the first 6-12 months
# of OSC's development.
#
# OSMV was originally hosted at https://github.com/adamkewley/osmv and later
# renamed to https://github.com/adamkewley/opensim-creator . This python script
# used to track the legacy repo and the current repo, but the legacy repo was
# gradually deprecated (first, with a warning message) and finally deleted to
# make it impossible for a user to accidently download a very very old version
# of the software. Because the repo is deleted, the API for the legacy counters
# no longer works, so their final numbers were baked into this script.
legacy_download_counters = [
    Row('osmv', '0.0.1', datetime.datetime.strptime('01-13-21', '%m-%d-%y'), 10),
    Row('osmv', '0.0.2', datetime.datetime.strptime('04-12-21', '%m-%d-%y'), 24),
    Row('osmv', '0.0.3', datetime.datetime.strptime('07-14-21', '%m-%d-%y'), 17),
    Row('osmv', '0.0.4', datetime.datetime.strptime('09-22-21', '%m-%d-%y'), 11),
    Row('osmv', '0.0.5', datetime.datetime.strptime('11-05-21', '%m-%d-%y'), 45),
    Row('osmv', '0.0.6', datetime.datetime.strptime('11-12-21', '%m-%d-%y'), 15),
    Row('osmv', '0.0.7', datetime.datetime.strptime('01-27-22', '%m-%d-%y'), 77),
]

urls = [
    "https://api.github.com/repos/ComputationalBiomechanicsLab/opensim-creator/releases?per_page=100",
]

version_pattern = re.compile(r"\d+\.\d+.\d+")

def fetch_raw_rows(add_legacy_rows=True):
    if add_legacy_rows:
        rv = legacy_download_counters.copy()
    else:
        rv = []
    for url in urls:
        resp = requests.get(url=url)

        for el in resp.json():
            for asset in el["assets"]:
                name = asset['name']
                match = version_pattern.search(name)
                if not match:
                    continue
                version = match.group(0)
                created_at = datetime.datetime.strptime(asset['created_at'], '%Y-%m-%dT%H:%M:%SZ')
                downloads = int(asset['download_count'])
                rv.append(Row(name, version, created_at, downloads))
    return rv

def extract_extension(row):
    name = row.name
    if match := version_pattern.search(name):
        name = row.name[match.end():]
    return '.'.join(name.split('.')[1:]) if '.' in name else ''

def print_download_analysis(rows):
    # aggregate downloads by version number
    aggregate = []
    for key, group in itertools.groupby(rows, lambda row: row.version):
        first = next(group)
        name = first.name.split('-')[0]
        version = first.version
        created_at = first.created_at
        downloads = functools.reduce(lambda acc,el: acc + el.downloads, group, first.downloads)
        aggregate.append(Row(name, version, created_at, downloads))
    rows = aggregate

    # sort by creation time
    rows = sorted(rows, key=lambda e: e.created_at)

    acc = 0
    print("download name,version,created at,num downloads,cumulative sum")
    for row in rows:
        acc += row.downloads
        print(f"{row.name},{row.version},{row.created_at.date()},{row.downloads},{acc}")

def print_download_type_percentages(rows):
    # filter out empty extensions
    rows = list(filter(lambda row: len(extract_extension(row))>0, rows))

    # sort by extension
    rows = sorted(rows, key=extract_extension)

    # aggregate by extension
    aggregate = []
    total_downloads = 0
    for key, group in itertools.groupby(rows, extract_extension):
        first = next(group)
        name = extract_extension(first)
        downloads = functools.reduce(lambda acc,el: acc + el.downloads, group, first.downloads)
        total_downloads += downloads
        aggregate.append((name, downloads))

    # print summary
    print('package usage:')
    for name, downloads in aggregate:
        print(f'    {name} = {100*downloads/total_downloads:.0f} %')

if __name__ == '__main__':
    rows = fetch_raw_rows()
    print_download_analysis(rows.copy())
    print_download_type_percentages(rows)
