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
    - [ ] Use `scripts/build_linux_debugging.sh` to clean-build a debug (+ libASAN)
          version of OSC on Ubuntu 24 (debugging os).
    - [ ] Ensure the `clang-tidy` lints and test suite passes with the debug build
    - [ ] Ensure the test suite passes under valgrind (see: `scripts/build_linux_valgrind.sh`)
    - [ ] Manually spot-check new changes with the debug+ASAN build
    - [ ] Fix all bugs/problems found during the above steps
    - [ ] Commit any fixes to CI and ensure CI passes
    - [ ] Clean-install the passing binaries on development machines, ensure they install on all OSes
          if there's any issues, fix them, commit, ensure CI passes, etc.
    - [ ] Update `CHANGELOG.md` sections such that the current `Unreleased` section becomes
          `XX.xx.pp` and add a new `Unreleased` section above that.
    - [ ] Tag+push the `CHANGELOG.md` update commit as a release (so that the commit message
          roughly matches something release-ey).
    - [ ] Rebase any currently-active feature branches onto the release commit (discourage stale branches)
    - [ ] Download release artifacts from the tagged commit CI build
      - [ ] Also, create a source tarball with `git archive --format=tar.xz --prefix=opensimcreator-${VERSION}/ -o opensimcreator-${VERSION}-src.tar.xz $VERSION` (or, on MacOS: git archive --format=tar --prefix=opensimcreator-${VERSION}/ $VERSION | xz > opensimcreator-${VERSION}-src.tar.xz)
      - [ ] You might need to configure `.tar.xz` support with `git config tar.tar.xz.command "xz -c"`
      - [ ] For MacOS, the release must be built on a developer's machine, and the developer should configure the build with codesigning+notarization and upload
            the signed+notarized binaries instead. See OSC_CODESIGN_ENABLED and OSC_NOTARIZATION_ENABLED flags in the
            MacOS packaging. Adam Kewley specifically has the CMake flags, password, keys etc. necessary to do this.
    - [ ] Unzip/rename any artifacts (see prev. releases)
    - [ ] Create new release on github from the tagged commit
      - [ ] Upload all artifacts against it
      - [ ] Write a user-friendly version of CHANGELOG as the release description that explains
            the release's changes to end-users in a readable way
    - [ ] Update Zenodo with the release
      - [ ] This usually happens automatically, via a webhook in Zenodo
      - [ ] Otherwise, it requires @adamkewley's GitHub login to publish
            the generated draft from Zenodo
    - [ ] Update + commit the repository with the Zenodo release details:
      - [ ] Use `./scripts/bump_zenodo_details.py` to automatically do this
      - [ ] Ensure `codemeta.json`, `CITATION.cff`, and `README.md` refer to the
            correct Zenodo release.
    - [ ] Ensure the entire repository, incl. all tags, is pushed to the official
          TU Delft mirror at https://gitlab.tudelft.nl/computationalbiomechanicslab/opensim-creator
    - [ ] Ensure all release artifacts, incl. the source tarball, are uploaded to
          `files.opensimcreator.com/releases`
      - [ ] Upload with (e.g.): `rsync --delete --exclude .git/ -avz files.opensimcreator.com/ files.opensimcreator.com:/var/www/files.opensimcreator.com/`
    - [ ] Update `docs.opensimcreator.com` to host the documentation
      - [ ] Build the docs yourself, or get the CI build of them
      - [ ] Upload with (e.g.) `rsync -avz --delete build/ docs.opensimcreator.com:/var/www/docs.opensimcreator.com/manual/en/latest/`
    - [ ] Update `www.opensimcreator.com` with a basic announcement news post
      - [ ] Edit https://github.com/ComputationalBiomechanicsLab/www.opensimcreator.com appropriately
      - [ ] Upload with (e.g.): `rsync -avz public/ www.opensimcreator.com:/var/www/opensimcreator.com/`
    - [ ] (optional) Update social media:
      - [ ] LinkedIn
      - [ ] Twitter
      - [ ] Bluesky
      - [ ] SimTK
      - [ ] Reddit (occasionally)
      - [ ] https://research-software-directory.org/ (usually this is automatic)

Release Build Matrix
--------------------

Here is a top-level overview of the architectures/platforms/compilers that we use
to produce OpenSim Creator's releases. Feature developers don't need to use exactly
the same combinations (it's healthy to exercise other combinations!) but these are
the ones that are used during a release, so everything must eventually compile+pass
with these combinations:

.. list-table:: OpenSim Creator's Release Matrix.
   :header-rows: 1

   * - Architecture
     - Target Operating System
     - C++ Compiler
   * - amd64
     - Windows 10 (>= v1507)
     - MSVC 19.43.34808.0 (VS 17.13.358, Windows SDK 10.0.26100.0)
   * - amd64
     - MacOS 14.5
     - XCode 15.4
   * - arm64
     - MacOS 14.5
     - XCode 15.4
   * - amd64
     - Ubuntu 22.04
     - gcc 12.3.0 (``gcc-12``, installed via ``apt``). For development, ``clang-18`` also works.

Notably, these toolchains **do not** have 100 % coverage of the C++20/23 language or
library specifications. So check `C++ Compiler Support`_ if you plan on using a newer C++
features.

Release Matrix Upgrades/Changes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Because it's exhausting to constantly update/change build pipelines (even if I'm *particularly*
desperate for ``std::format`` support 😉 - looking at you, ``gcc-12``), this build matrix should
only be checked/updated occasionally. The next scheduled times that we will check this matrix are:

- October 2025, which is when Windows 10 support is EOL. The build system will be upgraded
  to target Windows 11.
- May 2026, which is when Ubuntu 22.04 is EOL for unpaid customers. The build system will
  then be upgraded to target Ubuntu 24.04.
- September 2026, which is when MacOS 14 is likely to be unsupported. The build system will
  then be upgraded to target MacOS 15 (Sequoia).

The build matrix might also change because of upgrades/changes to the CI server. Those changes
will (hopefully) be mostly limited to minor bugfix upgrades.

.. _C++ Compiler Support: https://en.cppreference.com/w/cpp/compiler_support
