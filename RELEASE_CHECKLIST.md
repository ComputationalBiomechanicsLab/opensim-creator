# Release Checklist

> Typical steps required to finalize a release of OpenSim Creator


# Release Steps

- [ ] Create an issue called something like `Release XX.xx.pp`
- [ ] Copy this checklist into it
- [ ] Bump OSC's version number in `CMakeLists.txt` (`project`)
- [ ] Clean-build a debug (+ libASAN) version of OSC on Ubuntu 24 (debugging os).
      See `scripts/build_ubuntu.sh` etc. for a guide for this.
- [ ] Ensure the test suite passes with the debug build
  - [ ] Optionally, also ensure the test suite passes under valgrind (see: `scripts/build_linux_valgrind.sh`)
- [ ] Manually spot-check new changes with the debug+ASAN build
- [ ] Fix all bugs/problems found during the above steps
- [ ] Update `CHANGELOG.md` sections such that the current `Unreleased`
      section becomes `XX.xx.pp` and add a new `Unreleased` section
      above that
- [ ] Commit any fixes to CI and ensure CI passes
- [ ] Tag+push the commit as a release
- [ ] Rebase any currently-active feature branches to this commit (don't allow stale branches)
- [ ] Bump OSC's version number in `CMakeLists.txt` (`project`) to `$VERSION+1`
- [ ] Download artifacts from the tagged commit CI build
  - [ ] Also, create a source tarball with `./scripts/bundle_sources.sh $VERSION`
  - [ ] Also, build a MacOS ARM64 build locally from the release and upload it
- [ ] Clean-install artifacts on development machines, ensure they install as-expected
- [ ] Unzip/rename any artifacts (see prev. releases)
- [ ] Create new release on github from the tagged commit
  - [ ] Upload all artifacts against it
  - [ ] Write a user-friendly version of CHANGELOG that explains the release's
        changes
- [ ] Update Zenodo with the release
  - [ ] This usually happens automatically, via a webhook in Zenodo
  - [ ] Otherwise, it requires @adamkewley's GitHub login to publish
        the generated draft from Zenodo
- [ ] Update + commit the repository with the Zenodo release details:
  - [ ] Use `bump_zenodo_details.py` to automatically do this
  - [ ] Ensure `codemeta.json` is up-to-date
  - [ ] Ensure `CITATION.cff` is up-to-date
  - [ ] Ensure `README.md` is up-to-date
- [ ] **NEW**: Update TU Delft mirror of the repository
- [ ] Update `docs.opensimcreator.com` to host the documentation
  - [ ] **Note**: this requires appropriate credentials for `docs.opensimcreator.com`
  - [ ] Build the docs yourself, or get the CI build of them
  - [ ] Upload with (e.g.) `rsync -avz --delete build/ docs.opensimcreator.com:/var/www/docs.opensimcreator.com/manual/en/latest/`
- [ ] Update `www.opensimcreator.com` with a basic announcement news post
  - [ ] **Note**: this requires appropriate SSH credentials for `www.opensimcreator.com`
  - [ ] Edit https://github.com/ComputationalBiomechanicsLab/opensim-creator-site appropriately
  - [ ] Upload with (e.g.): `rsync -avz public/ www.opensimcreator.com:/var/www/opensimcreator.com/`
- [ ] Update `files.creator.com/releases` with appropriate release artifacts
  - [ ] Upload with (e.g.): `rsync --delete --exclude .git/ -avz files.opensimcreator.com/ files.opensimcreator.com:/var/www/files.opensimcreator.com/`
- [ ] (optional) Update social media:
  - [ ] LinkedIn
  - [ ] Twitter
  - [ ] Bluesky
  - [ ] SimTK
  - [ ] Reddit (occasionally)
  - [ ] https://research-software-directory.org/ (usually this is automatic)
  - [ ] Threads
