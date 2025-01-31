How to make a new release?
--------------------------

1. Ensure that the full version of nanobind is checked out (including the
   ``robin_map`` submodule)

2. Update version in ``src/__init__.py``, ``include/nanobind/nanobind.h``,
   and ``pyproject.toml``:

   Also:

   - Remove ``dev1`` suffix from ``X.Y.Zdev1`` in ``src/__init__.py`` and
     ``pyproject.toml``.

   - Set ``NB_VERSION_DEV`` to ``0`` in ``include/nanobind/nanobind.h``.

3. Add release date to ``docs/changelog.rst``.

4. Update ``setup.py`` if new directories were added (see ``package_data``).
   Update ``cmake/nanobind-config.cmake`` if new C++ source or header files
   were added.

5. Commit: ``git commit -am "vX.Y.Z release"``

6. Tag: ``git tag -a vX.Y.Z -m "vX.Y.Z release"``

7. Push: ``git push`` and ``git push --tags``

8. Run ``pipx run build``

9. Upload: ``twine upload --repository nanobind <filename>``

10. Update version in ``src/__init__.py`` and ``include/nanobind/nanobind.h``:

   - Append ``dev1`` suffix from ``X.Y.Zdev1`` in ``src/__init__.py`` and
     ``pyproject.toml``.

   - Set ``NB_VERSION_DEV`` to ``1`` in ``include/nanobind/nanobind.h``
