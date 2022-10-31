#!/usr/bin/env python

# prints out a CSV table containing OpenSim Creator's release download counts
#
# requires `requests`: `pip install requests`

import requests
import datetime

# this is where the download counters are pulled from (GitHub public API)
release_urls = [
  "https://api.github.com/repos/ComputationalBiomechanicsLab/opensim-creator/releases",
  "https://api.github.com/repos/adamkewley/opensim-creator/releases",
]

# a single row in the table
class Row:
    def __init__(self, name, created_at, downloads):
        self.name = name
        self.created_at = created_at
        self.downloads = downloads

# returns all rows for a particular release URL
def get_rows_for(release_url):

    r = requests.get(url=release_url)

    rows = []
    for el in r.json():
        for asset in el["assets"]:
            name = asset['name']
            created_at = datetime.datetime.strptime(asset['created_at'], '%Y-%m-%dT%H:%M:%SZ')
            downloads = asset['download_count']
            rows.append(Row(name, created_at, downloads))
    return rows

# prints the given sequence of rows to the standard output
def print_rows(rows):
    acc = 0
    print("download name,created at,num downloads,cumulative sum")
    for row in rows:
        acc += row.downloads
        print(f"{row.name},{row.created_at.date()},{row.downloads},{acc}")


def main():

    # get all release rows
    rows = []
    for release_url in release_urls:
        rows += get_rows_for(release_url)

    # sort by date
    rows = sorted(rows, key=lambda e: e.created_at)

    # print to stdout
    print_rows(rows)

if __name__ == '__main__':
    main()
