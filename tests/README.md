# `tests/`

> Test suite for `OpenSimCreator` and `oscar`

Uses `googletest` to test various parts of the project.


## Note: Debugging on Visual Studio

`googletest` catches and reports Windows SEH exceptions. This is handy when (e.g.)
running the test suites from a commandline, where you *probably* just want a report
saying which ones have failed, but it is problematic if you specifically are running
the test suite to debug a runtime error that the test is exercising.

To disable this behavior, you have to provide `--gtest_catch_exceptions=0` to the
test suite executable. In Visual Studio you can do this by:

- Setting the executable as your startup (`F5` et. al.) project
- Then go to `Debug > Debug and Launch Settings for TestOpenSimCreator.exe` (for example)
- And then add an `args` element to the `configurations` list

Example:

```json
{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "TestOpenSimCreator.exe (tests\\TestOpenSimCreator\\TestOpenSimCreator.exe)",
      "name": "TestOpenSimCreator.exe (tests\\TestOpenSimCreator\\TestOpenSimCreator.exe)",
      "args": [
        "--gtest_catch_exceptions=0"
      ]
    }
  ]
}
```
