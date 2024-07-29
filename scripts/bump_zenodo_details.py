#!/usr/bin/env python3
#
# `bump_zenodo_details`: automatically updates relevant source files in
# the repository (e.g. the README, CITATION.cff) to point at the latest
# Zenodo release
#
#     usage: `python bump_zenodo_details.py` (follow the prompts)
#            `git diff` (ensure it made sane changes)

import datetime
import re
import subprocess

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
    default_commit = subprocess.run(f'git rev-list -n 1 {new_version}', stdout=subprocess.PIPE).stdout.decode('utf-8').strip()
    commit = _input_with_default('commit', default_commit)

    return BumperInputs(citation_string, doi_url, new_version, date_published, commit)

def _slurp_file(path):
    with open(path, 'r', encoding='utf-8') as fd:
        return fd.read()

def _overwrite_file(path, new_content):
    with open(path, 'w', encoding='utf-8') as fd:
        fd.write(new_content)

def _update_readme(inputs: BumperInputs):
    content = _slurp_file('README.md')
    new_content = re.sub(r'^>.+?https://doi.org/[^/]+/zenodo.+?$', f'> {inputs.citation_string}', content, 0, re.MULTILINE)
    _overwrite_file('README.md', new_content)

def _update_citation_cff(inputs: BumperInputs):
    content = _slurp_file('CITATION.cff')
    new_content = re.sub(r'value: \d+\.\d+/zenodo\.\d+', f'value: {inputs.doi}', content)
    new_content = re.sub(r'^commit: .+?$', f'commit: {inputs.commit}', new_content, 0, re.MULTILINE)
    new_content = re.sub(r'^version: .+?$', f'version: {inputs.new_version}', new_content, 0, re.MULTILINE)
    new_content = re.sub(r'^date-released: .+?$', f"date-released: '{inputs.date_published}'", new_content, 0, re.MULTILINE)
    new_content = re.sub(r'entry containing .+? binaries', f'entry containing {inputs.new_version} binaries', new_content)
    _overwrite_file('CITATION.cff', new_content)

def _update_codemeta_json(inputs: BumperInputs):
    content = _slurp_file('codemeta.json')
    new_content = re.sub(r'"https://doi.org.+?"', f'"{inputs.doi_url}"', content)
    new_content = re.sub(r'"datePublished": ".+?"', f'"datePublished": "{inputs.date_published}"', new_content)
    new_content = re.sub(r'"version": ".+?"', f'"version": "{inputs.new_version}"', new_content)
    _overwrite_file('codemeta.json', new_content)

def main():
    inputs = _collect_inputs_from_user()
    _update_readme(inputs)
    _update_citation_cff(inputs)
    _update_codemeta_json(inputs)

if __name__ == '__main__':
    main()
