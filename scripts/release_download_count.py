import datetime
import itertools
import functools
import requests
import re

class Row:
    def __init__(self, name, version, created_at, downloads):
        self.name = name
        self.version = version
        self.created_at = created_at
        self.downloads = downloads

urls = [
    "https://api.github.com/repos/adamkewley/opensim-creator/releases",
    "https://api.github.com/repos/ComputationalBiomechanicsLab/opensim-creator/releases",
]

version_pattern = re.compile("\d+\.\d+.\d+")

rows = []
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
            rows.append(Row(name, version, created_at, downloads))

# aggregate downloads by version number
aggregate = []
for key, group in itertools.groupby(rows, lambda row: row.version):
    first = next(group)
    name = first.version
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
