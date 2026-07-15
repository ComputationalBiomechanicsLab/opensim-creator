The Release Process
===================

This is a rough overview of OPynSim's release process - mostly intended
as internal documentation for the development team.

OPynSim aims to have a  1-3 month release cadence - even if no major
changes occur in a release window. This ensures that at least one commit
of OPynSim's source code is tested, libASANed, linted, manually-checked, etc.
every 1-3 months.

Release Checklist (Markdown)
----------------------------

This release checklist is in markdown so that it can be copied + pasted into
websites like GitHub.

.. code:: markdown

    ## Prepare, Test, Package

    - [ ] Create an issue called something like `Release XX.xx.pp`
    - [ ] Copy this checklist into it
    - [ ] Bump the VERSION tag in the top-level `CMakeLists.txt` file to `XX.xx.pp`
    - [ ] On a Linux machine, clean-build the `ErrorCheck` configuration `cd third_party/ && cmake --workflow --preset ErrorCheck && cd - && cmake --workflow --preset ErrorCheck`
    - [ ] Ensure the `ErrorCheck` build + tests all pass
    - [ ] Manually spot-check any new changes with the `ErrorCheck` build
    - [ ] Fix all bugs/problems/lints found during the above steps
    - [ ] Commit the fixes to CI and ensure CI passes
    - [ ] Download CI artifacts for all supported platforms. Install them on all applicable test/development machines. Manually spot-check that they install, work, etc.
    - [ ] Update  `CHANGELOG.md`:
      - [ ] Move sections around such that `Unreleased` becomes `XX.xx.pp`, add a new (empty) `Unreleased` section at the top.
      - [ ] Write a very short summary paragraph at the top of the `XX.xx.pp` section, for use in GitHub, website, etc.
    - [ ] Push the `CHANGELOG.md` update commit (e.g. "Update CHANGELOG.md for XX.xx.pp")
    - [ ] Tag and push the commit as `v${VERSION}`
    - [ ] Rebase any active branches onto the `main` branch so that all branches are at-least compatible with the latest release, or delete the branches if they are stale.
    - [ ] Collect all build artifacts:
      - [ ] `opynsim-${VERSION}-cp312-abi3-macosx_14_0_arm64.whl`
      - [ ] `opynsim-${VERSION}-cp312-abi3-win_amd64.whl`
      - [ ] `opynsim-${VERSION}-cp312-abi3-manylinux_2_28_x86_64.whl`
      - [ ] `opynsim-${VERSION}-docs.tar.xz`: E.g. `tar cvzf opynsim-${VERSION}-docs.tar.xz -C ${DOCS_HTML_DIR} --transform="s,^\.,opynsim-${VERSION}-docs," .`.
      - [ ] `opynsim-${VERSION}-src.tar.xz`: E.g. `git archive --format=tar --prefix=opynsim-${VERSION}/ v${VERSION} | xz -c > "opynsim-${VERSION}-src.tar.xz"`.

    ## Publish

    - [ ] Create a GitHub release from the tagged commit
      - [ ] Upload all artifacts build artifacts against the release
      - [ ] Copy + paste the release summary paragraph from `CHANGELOG.md` as the release description
    - [ ] Upload wheels to PyPi
      - [ ] Set `TWINE_USERNAME` and `TWINE_PASSWORD` (stored on developer's keychain)
      - [ ] Run `./scripts/deploy_pypi.py WHEEL_FILES` (or similar)
    - [ ] Update Zenodo with the release (https://zenodo.org/records/21373996):
      - [ ] This should happen automatically, because Zenodo is linked to @adamkewley, which
            and has linked/mirrored organizational OAuth access for OPynSim setup at the
            user-level.
    - [ ] Update `README.md` with Zenodo release details
    - [ ] Ensure the entire repository is pushed/mirrored to the official TU Delft mirror:
      - [ ] `git remote add tud https://gitlab.tudelft.nl/opynsim/opynsim.git`
      - [ ] `git push tud --all`
      - [ ] `git push tud --tags`
    - [ ] Ensure all build artifacts are uploaded to `files.opynsim.eu/releases`:
      - [ ] `rsync -avz release-binaries/* files.opynsim.eu:/var/www/files.opynsim.eu/releases/`
    - [ ] Upload built documentation to `docs.opynsim.eu`
      - [ ] `rsync -avz --delete ${DOCS_HTML_DIR}/ docs.opynsim.eu:/var/www/docs.opynsim.eu/manual/en/latest`
    - [ ] Ensure repo syncs with research-software-directory.org (https://research-software-directory.org/software/opynsim)

    ## Announce Release

    - [ ] Make short announcement posts on social media:
      - [ ] LinkedIn
      - [ ] Bluesky
      - [ ] SimTK

    - [ ] Ensure any collaborators/researchers involved with the release cycle are
          informed of the release.
