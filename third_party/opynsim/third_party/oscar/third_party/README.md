# third_party: Superbuild for oscar's dependencies

Note: some third-party code might be patched, check `patches/` and apply them
after updating a third-party upstream. Here's how you apply a patch

```bash
# e.g. for ImGui

cd imgui
patch -p1 < ../patches/imgui/0001-some-patch.patch
```

After patching, commit the patched version. The reason we do it this way is because
it's annoying to patch things as part of a cross-platform CMake build and we don't
want to patch it in-tree with `git` or similar because then the working tree is dirty
and has to be screwed around with.

