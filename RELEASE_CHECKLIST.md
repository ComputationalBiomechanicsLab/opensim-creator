# Release Checklist

> Typical steps required to finalize a release of OpenSim Creator


# Required Credentials

These are required for some parts of the release procedure:

- Zenodo is currently linked (via OAuth) to @adamkewley's GitHub account
- Uploading the documentation to `docs.opensimcreator.com` requires a suitable
  SSH key for that server
- Adding an announcement to the website requires commit access to
  [opensim-creator-site](https://github.com/ComputationalBiomechanicsLab/opensim-creator-site)
- Uploading the announcement to `www.opensimcreator.com` requires a suitable
  SSH key for that server
- Uploading archived releases to `files.opensimcreator.com` requires a suitable
  SSH key for that server


# Release Steps

- [ ] Create an issue called something like `Release XX.xx.pp`
- [ ] Copy this checklist into it
- [ ] Bump OSC's version number in `CMakeLists.txt` (`project`)
- [ ] Clean-build a debug (+ libASAN) version of OSC on Ubuntu:

```bash
# Ubuntu > 20.04:
# WSL2 on Ubuntu24: ln -sf  /mnt/wslg/runtime-dir/wayland-* $XDG_RUNTIME_DIR/

sudo apt install clang clang++ clang-tidy cmake pkg-config libgtk-3-dev libblas-dev liblapack-dev
git clone --recurse-submodules https://github.com/ComputationalBiomechanicsLab/opensim-creator
cd opensim-creator
./scripts/build_linux_debugging.sh
```

```bash
# Ubuntu 20.04:
#!/usr/bin/env bash

# setup system dependencies
sudo apt update
sudo apt install -y cmake pkg-config libgtk-3-dev libblas-dev liblapack-dev clang-11 clang-tidy-11 libstdc++-10-dev
export CC=clang-11
export CXX=clang++-11

# install a newer cmake
sudo apt install -y libssl-dev  # OpenSSL headers, for cmake
wget https://github.com/Kitware/CMake/releases/download/v3.31.2/cmake-3.31.2.tar.gz
tar xvf cmake-3.31.2.tar.gz
cd cmake-3.31.2
./bootstrap
make -j$(nproc) && sudo make install
cd -

export CLANG_TIDY=echo  # clang-tidy-11 is screwed on ubuntu11
git clone --recurse-submodules https://github.com/ComputationalBiomechanicsLab/opensim-creator
cd opensim-creator
./scripts/build_linux_debugging.sh
```

- [ ] Ensure the test suite passes with the debug build
  - [ ] Optionally, also ensure the test suite passes under valgrind (see: scripts/build_linux_valgrind-compat.sh)
- [ ] Manually spot-check new changes with the debug+ASAN build
- [ ] Fix all bugs/problems found during the above steps
- [ ] Update `CHANGELOG.md` sections such that the current `Unreleased`
      section becomes `XX.xx.pp` and add a new `Unreleased` section
      above that
- [ ] Commit any fixes to CI and ensure CI passes
- [ ] Tag+push the commit as a release
- [ ] Download artifacts from the tagged commit CI build
- [ ] Clean-install artifacts on development machines, ensure they install as-expected
- [ ] Unzip/rename any artifacts (see prev. releases)
- [ ] Create new release on github from the tagged commit
  - Upload all artifacts against it
  - Write a user-friendly version of CHANGELOG that explains the release's
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
- [ ] Update `docs.opensimcreator.com` to host the documentation
  - [ ] **Note**: this requires appropriate credentials for `docs.opensimcreator.com`
  - [ ] Build the docs yourself, or get the CI build of them
  - [ ] Upload with (e.g.) `rsync -avz build/html/ docs.opensimcreator.com:/var/www/docs.opensimcreator.com/manual/en/VERSION`
  - [ ] Make them the "latest" with a softlink, e.g.: `ssh docs.opensimcreator.com ln -sfn /var/www/docs.opensimcreator.com/manual/en/VERSION /var/www/docs.opensimcreator.com/manual/en/latest`
- [ ] (optional) Update `www.opensimcreator.com` with a basic announcement news post
  - [ ] **Note**: this requires appropriate SSH credentials for `www.opensimcreator.com`
  - [ ] Edit https://github.com/ComputationalBiomechanicsLab/opensim-creator-site appropriately
  - [ ] Upload with (e.g.): `rsync -avz public/* www.opensimcreator.com:/var/www/opensimcreator.com/`
- [ ] (optional) Update `files.creator.com/releases` with appropriate release artifacts
- [ ] (optional) Update social media:
  - [ ] LinkedIn
  - [ ] Twitter
  - [ ] Bluesky
  - [ ] SimTK
  - [ ] Reddit (occasionally)
