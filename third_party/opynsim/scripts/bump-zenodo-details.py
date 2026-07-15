#!/usr/bin/env python3
#
# Automatically updates relevant source files in the repository (e.g. the
# README, CITATION.cff) to point at the latest Zenodo release.
#
# USAGE:
#
#     ${THIS}                                 # follow the prompts this script gives
#     git diff                                # ensure the script made sane changes
#     git add -A                              # index the changes
#     git commit -m "Updated Zenodo details"  # commit changes

import datetime
import re
import subprocess
from pathlib import Path

class BumperInputs:
    def __init__(self, citation_string, doi_url, new_version, date_published, commit):
        self.citation_string = citation_string
        self.doi_url = doi_url
        self.doi = re.sub('https://doi.org/', '', self.doi_url)
        self.new_version = new_version
        self.date_published = date_published
        self.commit = commit

def _input_with_default(prompt, default_val):
    response = input(f'{prompt} [{default_val}]: ').strip()
    if response:
        return response
    else:
        return default_val

def _collect_inputs_from_user() -> BumperInputs:
    # e.g. Kewley, A., Beesel, J., & Seth, A. (2023). OpenSim Creator (0.5.6). Zenodo. https://doi.org/10.5281/zenodo.10427138
    citation_string = input('citation string: ')
    doi_url = re.search(r'https://.+$', citation_string).group(0)
    default_new_version = re.search(r'[0-9]+.[0-9]+.[0-9]+', citation_string).group(0)

    # e.g. 0.5.7
    new_version = _input_with_default('new version', default_new_version)

    # e.g. 2023-12-22
    default_date = datetime.datetime.now().strftime('%Y-%m-%d')
    date_published = _input_with_default('date published', default_date)

    # e.g. 7fdbd6c231dd582c16c68d343cc39c38d0a487f3
    default_commit = subprocess.run(f'git rev-list -n 1 {new_version}', shell=True, stdout=subprocess.PIPE).stdout.decode('utf-8').strip()
    commit = _input_with_default('commit', default_commit)

    return BumperInputs(citation_string, doi_url, new_version, date_published, commit)

def _update_readme(inputs: BumperInputs):
    readme_md = Path("README.md")
    content = readme_md.read_text()
    new_content = re.sub(r'^>.+?https://doi.org/[^/]+/zenodo.+?$', f'> {inputs.citation_string}', content, flags=re.MULTILINE)
    readme_md.write_text(new_content)

def _update_citation_cff(inputs: BumperInputs):
    citation_cff = Path("CITATION.cff")
    if not citation_cff.exists():
        print(f"{citation_cff} does not exist: skipping")
        return
    content = citation_cff.read_text()
    new_content = re.sub(r'value: \d+\.\d+/zenodo\.\d+', f'value: {inputs.doi}', content)
    new_content = re.sub(r'^commit: .+?$', f'commit: {inputs.commit}', new_content, flags=re.MULTILINE)
    new_content = re.sub(r'^version: .+?$', f'version: {inputs.new_version}', new_content, flags=re.MULTILINE)
    new_content = re.sub(r'^date-released: .+?$', f"date-released: '{inputs.date_published}'", new_content, flags=re.MULTILINE)
    new_content = re.sub(r'entry containing .+? binaries', f'entry containing {inputs.new_version} binaries', new_content)
    citation_cff.write_text(new_content)

def _update_codemeta_json(inputs: BumperInputs):
    codemeta_json = Path("codemeta.json")
    if not codemeta_json.exists():
        print(f"{codemeta_json}: does not exist: skipping")
        return
    content = codemeta_json.read_text()
    new_content = re.sub(r'"https://doi.org.+?"', f'"{inputs.doi_url}"', content)
    new_content = re.sub(r'"datePublished": ".+?"', f'"datePublished": "{inputs.date_published}"', new_content)
    new_content = re.sub(r'"version": ".+?"', f'"version": "{inputs.new_version}"', new_content)
    codemeta_json.write_text(new_content)

def main():
    inputs = _collect_inputs_from_user()
    _update_readme(inputs)
    _update_citation_cff(inputs)
    _update_codemeta_json(inputs)

if __name__ == '__main__':
    main()
