Release Process
===============

This is a rough overview of OpenSim Creator's release process - mostly intended as
internal documentation for the development team.

.. note::

    As of OpenSim Creator 0.9.0, most of OpenSim Creator's source code and build
    was ported to `OPynSim <https://github.com/opynsim/opynsim>`_ to make the developers'
    lives easier (developing across two repos is a nightmare). The release process for
    OpenSim Creator is mostly about packaging OPynSim to resemble OpenSim
    Creator's previous packaging.

    If you need to setup a development environment to build OpenSim Creator from
    source on your computer then consult the OPynSim development guide at
    https://docs.opynsim.eu/.

Release Checklist (Markdown)
----------------------------

This checklist outlines the manual steps taken to produce a release of OpenSim
Creator, it's usually copied into a GitHub issue:

.. code-block:: markdown

    - [ ] The version number matches OPynSim's version (lockstep: both projects are locked together by one
          team. The version number shown to users in osc.exe is _OPynSim's_ because that's where the GUI
          is built).
    - [ ] Create an issue called something like `Release XX.xx.pp`
    - [ ] Copy this checklist into it
    - [ ] Bump the VERSION in the top-level `CMakeLists.txt` accordingly
    - [ ] Clean-install the passing binaries on development machines, ensure they install on all OSes
          if there's any issues, fix them, commit, ensure CI passes, etc.
    - [ ] Update `CHANGELOG.md` sections such that the current `Unreleased` section becomes
          `XX.xx.pp` and add a new `Unreleased` section above that. Add a release summary
          paragraph to the top of the new version's section.
    - [ ] Tag+push the `CHANGELOG.md` update commit as a release (so that the commit message
          roughly matches something release-ey).
    - [ ] Rebase any currently-active feature branches onto the release commit (discourage stale branches)
    - [ ] Assemble/build/sign release artifacts:
      - [ ] Download source tarball, or create it with `cpack --config CPackSourceConfig.cmake` in the build directory.
      - [ ] Download documentation tarball, it's also created by the `Release-Linux-amd64` build
      - [ ] Build MacOS release on developer's machine (GitHub Actions doesn't store developer's private keys):
        - [ ] Setup local Python 3.12 virtual environment with something like `python3.12 -m venv .venv && source .venv/bin/activate && pip install -r requirements/all_requirements.txt`
        - [ ] Ensure secret codesigning environment variables are set: `OSC_CODESIGN_DEVELOPER_ID`,
              `OSC_NOTARIZATION_APPLE_ID`, `OSC_NOTARIZATION_TEAM_ID`, and `OSC_NOTARIZATION_PASSWORD`.
        - [ ] Build and notarize an **arm64** release on the developer's machine with `cmake --workflow --preset Release-MacOS-arm64-CodesignedAndNotarized`
      - [ ] Build Windows release on developer's machine (GitHub Actions cannot access physical signing USB keys):
        - [ ] Setup local Python virtual environment with something like `py -3.12 -m venv . venv && call .venv\Scripts\activate && pip install -r requirements/all_requirements.txt`
        - [ ] Build and codesign an **amd64** release on the developer's machine by building the `Release-Windows-amd64-Codesigned` cmake workflow (use `Release-Windows-amd64` third-party build).
      - [ ] Combine all artifacts into a single location/directory:
        - [ ] Source tarball
        - [ ] Documentation tarball
        - [ ] Linux DEB package
        - [ ] Linux portable .tar.xz
        - [ ] MacOS codesigned and notarized arm64 dmg
        - [ ] Windows codesigned amd64 msi
        - [ ] Windows codesigned amd64 portable zip
    - [ ] Create new release on github from the tagged commit:
      - [ ] Upload all artifacts against it
      - [ ] Copy + paste the release summary paragraph from `CHANGELOG.md`, maybe modify the
            note a little for GitHub-specific stuff (e.g. links to things).
    - [ ] Update Zenodo with the release (https://zenodo.org/records/24498452):
      - [ ] This usually happens automatically, via a webhook in Zenodo
      - [ ] Otherwise, it requires @adamkewley's GitHub login to publish
            the generated draft from Zenodo
    - [ ] Update + commit the repository with the Zenodo release details:
      - [ ] Use `./third_party/opynsim/scripts/bump-zenodo-details.py` to automatically do this
      - [ ] Ensure `codemeta.json`, `CITATION.cff`, and `README.md` refer to the
            correct Zenodo release.
    - [ ] Ensure the entire repository, incl. all tags, is pushed to the official
          TU Delft mirror at https://gitlab.tudelft.nl/computationalbiomechanicslab/opensim-creator
      - [ ] `git remote add gitlab https://gitlab.tudelft.nl/computationalbiomechanicslab/opensim-creator.git`
      - [ ] `git push gitlab main && git push gitlab TAG`
    - [ ] Ensure all release artifacts are uploaded to:
          `files.opensimcreator.com/releases`
      - [ ] Upload with (e.g.): `rsync --delete --exclude .git/ -avz files.opensimcreator.com/ files.opensimcreator.com:/var/www/files.opensimcreator.com/`
    - [ ] Update `docs.opensimcreator.com` to host the documentation:
      - [ ] Unpack the documentation tarball
      - [ ] Deploy the documentation (also possible with the `opensimcreator_docs_deploy` target): requires server credentials.
    - [ ] (optional) Update social media:
      - [ ] LinkedIn
      - [ ] Twitter
      - [ ] Bluesky
      - [ ] SimTK
      - [ ] Reddit (occasionally)
      - [ ] https://research-software-directory.org/software/opensim-creator (usually this is automatic)
