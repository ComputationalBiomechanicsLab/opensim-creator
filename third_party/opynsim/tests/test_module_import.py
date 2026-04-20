import opynsim  # i.e. this should import fine

def test_can_run_python_script_that_imports_opynsim():
    # The only thing this test is testing is whether the test harness
    # is capable of loading a module that only `import`s `opynsim`.
    assert True
