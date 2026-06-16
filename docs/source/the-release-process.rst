The Release Process
===================

This is a rough overview of OpenSim Creator's release process - mostly intended as
internal documentation for the development team.

OpenSim Creator aims to have a 1-2 month release cadence - even if no major changes
occur in that window. This ensures that at least one tested, libASANed, linted, and
manually-checked release of OpenSim Creator available with that cadence and ensures
that the release pipeline is regularly exercised and ready to roll when quick hotfixes
are required.

Release Checklist (Markdown)
----------------------------

This checklist outlines the manual steps taken to produce a release of OpenSim
Creator, it's usually copied into a GitHub issue:

.. code-block:: markdown

    - [ ] Create an issue called something like `Release XX.xx.pp`
    - [ ] Copy this checklist into it
    - [ ] Bump the VERSION in the top-level `CMakeLists.txt` accordingly
    - [ ] Build the project with `ErrorCheck` on Ubuntu 24 and ensure all tests etc. pass
          with it. Must be ran with `ctest --preset ErrorCheck` because there's ASAN
          options (suppressions) that must be handled.
    - [ ] Ensure the `clang-tidy` lints and test suite passes with the debug build
    - [ ] Ensure the test suite passes under `valgrind`: `LIBGL_ALWAYS_SOFTWARE=1 valgrind --leak-check=full --trace-children=yes --suppressions=${PWD}/scripts/suppressions_valgrind.supp -- ctest --test-dir build/RelWithDebInfo --output-on-failure`
    - [ ] Manually spot-check new changes with the debug+ASAN build
    - [ ] Fix all bugs/problems found during the above steps
    - [ ] Commit any fixes to CI and ensure CI passes
    - [ ] Clean-install the passing binaries on development machines, ensure they install on all OSes
          if there's any issues, fix them, commit, ensure CI passes, etc.
    - [ ] Update `CHANGELOG.md` sections such that the current `Unreleased` section becomes
          `XX.xx.pp` and add a new `Unreleased` section above that. Add a release summary
          paragraph to the top of the new version's section.
    - [ ] Tag+push the `CHANGELOG.md` update commit as a release (so that the commit message
          roughly matches something release-ey).
    - [ ] Rebase any currently-active feature branches onto the release commit (discourage stale branches)
    - [ ] Assemble/build/sign release artifacts:
      - [ ] Create a source tarball with `git archive --format=tar --prefix=opensimcreator-${VERSION}/ ${VERSION} | xz -c > "opensimcreator-${VERSION}-src.tar.xz"`
      - [ ] Build MacOS release on developer's machine (GitHub Actions doesn't store developer's private keys):
        - [ ] Setup local Python 3.12 virtual environment with something like `python3.12 -m venv .venv && source .venv/bin/activate && pip install -r requirements/all_requirements.txt`
        - [ ] Ensure secret codesigning environment variables are set: `OSC_CODESIGN_DEVELOPER_ID`,
              `OSC_NOTARIZATION_APPLE_ID`, `OSC_NOTARIZATION_TEAM_ID`, and `OSC_NOTARIZATION_PASSWORD`.
        - [ ] Build and notarize an **arm64** release on the developer's machine with `OSC_CODESIGN_ENABLED=1 OSC_NOTARIZATION_ENABLED=1 ./scripts/ci_build_unix.sh Release-MacOS-arm64`
        - [ ] Build and notarize an **amd64** release on the developer's machine with `OSC_CODESIGN_ENABLED=1 OSC_NOTARIZATION_ENABLED=1 ./scripts/ci_build_unix.sh Release-MacOS-amd64`
      - [ ] Build Windows release on developer's machine (GitHub Actions cannot access physical signing USB keys):
        - [ ] Setup local Python virtual environment with something like `py -3.12 -m venv . venv && call .venv\Scripts\activate && pip install -r requirements/all_requirements.txt`
        - [ ] Build and codesign an **amd64** release on the developer's machine by building the `Release-Windows-amd64-Codesigned` cmake workflow (use `Release-Windows-amd64` third-party build).
      - [ ] Combine all artifacts into a single location/directory:
        - [ ] Source tarball
        - [ ] Linux DEB package
        - [ ] MacOS codesigned and notarized arm64 dmg
        - [ ] MacOS codesigned and notarized amd64 dmg
        - [ ] Windows codesigned amd64 msi
        - [ ] Windows amd64 portable zip
    - [ ] Create new release on github from the tagged commit
      - [ ] Upload all artifacts against it
      - [ ] Copy + paste the release summary paragraph from `CHANGELOG.md`, maybe modify the
            note a little for GitHub-specific stuff (e.g. links to things).
    - [ ] Update Zenodo with the release (https://zenodo.org/records/18701339):
      - [ ] This usually happens automatically, via a webhook in Zenodo
      - [ ] Otherwise, it requires @adamkewley's GitHub login to publish
            the generated draft from Zenodo
    - [ ] Update + commit the repository with the Zenodo release details:
      - [ ] Use `./scripts/oneoff_bump-zenodo-details.py` to automatically do this
      - [ ] Ensure `codemeta.json`, `CITATION.cff`, and `README.md` refer to the
            correct Zenodo release.
    - [ ] Ensure the entire repository, incl. all tags, is pushed to the official
          TU Delft mirror at https://gitlab.tudelft.nl/computationalbiomechanicslab/opensim-creator
      - [ ] `git remote add gitlab https://gitlab.tudelft.nl/computationalbiomechanicslab/opensim-creator.git`
      - [ ] `git push gitlab main && git push gitlab TAG`
    - [ ] Ensure all release artifacts, incl. the source tarball, are uploaded to
          `files.opensimcreator.com/releases`
      - [ ] Upload with (e.g.): `rsync --delete --exclude .git/ -avz files.opensimcreator.com/ files.opensimcreator.com:/var/www/files.opensimcreator.com/`
    - [ ] Update `docs.opensimcreator.com` to host the documentation
      - [ ] Build the docs (e.g. build the `opensimcreator_docs` target), or get the CI build of them.
      - [ ] Deploy the docs (e.g. build the `opensimcreator_docs_deploy` target): requires server credentials.
    - [ ] Update `www.opensimcreator.com` with a basic announcement news post
      - [ ] Edit https://github.com/ComputationalBiomechanicsLab/www.opensimcreator.com appropriately
      - [ ] Build the docs yourself with `hugo`
      - [ ] Upload with: `./scripts/deploy.sh` (available in the site repo)
    - [ ] (optional) Update social media:
      - [ ] LinkedIn
      - [ ] Twitter
      - [ ] Bluesky
      - [ ] SimTK
      - [ ] Reddit (occasionally)
      - [ ] https://research-software-directory.org/software/opensim-creator (usually this is automatic)

Release Build Matrix
--------------------

Here is a **rough** top-level overview of the architectures/platforms/compilers that
are used to produce OpenSim Creator's releases. Developers don't need to use exactly
the same combinations (it's healthy to exercise other combinations!) but these are
the ones that are used during a release, so everything must eventually compile+pass
with these combinations:

.. list-table:: OpenSim Creator's Release Matrix.
   :header-rows: 1

   * - Architecture
     - Target Operating System
     - C++ Toolchain Used
   * - amd64
     - Windows 10 (>= v1507)
     - MSVC 19.44.35227.0
   * - amd64
     - MacOS 14.5
     - XCode 15.4
   * - arm64
     - MacOS 14.5
     - XCode 15.4
   * - amd64
     - Linux /w glibc 2.28 (e.g. Ubuntu >=20.04, Debian >=10, AlmaLinux >=8, RHEL >=8, OpenSUSE >=15)
     - gcc 15 (e.g. ``gcc-toolset-15-gcc`` in AlmaLinux8)

Notably, these toolchains **do not** have 100 % coverage of the C++20/23 language or
library specifications. So check `C++ Compiler Support`_ if you plan on using a newer C++
features.

.. _C++ Compiler Support: https://en.cppreference.com/w/cpp/compiler_support
