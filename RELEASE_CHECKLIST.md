# Release Checklist

These are the typical steps required to finalize a release of OpenSim
Creator.

How comprehensively the QA/testing steps are done is entirely dependent
on how many changes were made since the last release (assuming an automated
test suite isn't available that covers the changes).

- [ ] Create an issue called something like `Release XX.xx.pp`
- [ ] Copy this checklist into it
- [ ] Bump OSC's version number in `CMakeLists.txt` (`project`)
- [ ] Clean-build a debug version of OSC:

```bash
git clone --recurse-submodules https://github.com/ComputationalBiomechanicsLab/opensim-creator
cd opensim-creator
./scripts/build_linux_debugging.sh
```

- [ ] Ensure test suite passes with debug+ASAN build
- [ ] Manually spot-check new changes with debug+ASAN build
- [ ] Fix all bugs/problems found during the above steps
- [ ] Update `CHANGELOG.md` sections such that the current `unreleased`
      section becomes `XX.xx.pp` and then add a new `unreleased` section
      above that
- [ ] Commit any fixes to CI and ensure CI passes
- [ ] Tag+push the commit as a release
- [ ] Download artifacts from the tagged commit CI build
- [ ] Unzip/rename any artifacts (see prev. releases)
- [ ] Create new release on github from the tagged commit
  - Upload all artifacts against it
  - Write a user-friendly version of CHANGELOG that explains changes
- [ ] Update Zenodo with the release (requires adamkewley's Zenodo login
      to publish the automatically generated draft)
- [ ] Update content with Zenodo details:
  - [ ] Use `bump_zenodo_details.py` to automatically do this
  - [ ] Ensure `codemeta.json` is up-to-date
  - [ ] Ensure `CITATION.cff` is up-to-date
  - [ ] Ensure `README.md` is up-to-date
