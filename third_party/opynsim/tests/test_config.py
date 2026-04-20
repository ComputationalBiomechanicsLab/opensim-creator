import logging
import multiprocessing as mp
import opynsim as opyn
import opynsim.config
from pathlib import Path

# Note: most functions in `opynsim.config` affect behavior globally,
# so they must be executed via a process fork (e.g. with `multiprocessing`
# or `subprocess`) so that they don't impact other tests.

# Generic worker function that executes `callable` with `args` and
# communicates its return value or `Exception` via a
# `multiprocessing.Queue`.
def run_callable_and_respond(result_queue, callable, args, kwargs):
    try:
        result_queue.put(callable(*args, **kwargs))
    except Exception as e:
        result_queue.put(e)

# Evaluates `callable` with `*args` in a separate process and synchronously
# joins + returns/raises the return value to the caller.
def execute_forked(callable, *args, **kwargs):
    result_queue = mp.Queue()
    process = mp.Process(target=run_callable_and_respond, args=(result_queue, callable, args, kwargs))
    process.start()
    process.join(timeout=30)
    if process.is_alive():
        process.terminate()
        raise TimeoutError("Subprocess timed out")
    result = result_queue.get()
    if isinstance(result, Exception):
        raise result
    return result

def test_default_log_level_is_WARN():
    assert opyn.config.get_log_level() == logging.WARN

def _set_log_level_then_assert_get_log_level():
    assert opyn.config.get_log_level() == logging.WARN
    opyn.config.set_log_level(logging.INFO)
    assert opyn.config.get_log_level() == logging.INFO

def test_set_log_level_makes_get_log_level_return_new_log_level():
    execute_forked(_set_log_level_then_assert_get_log_level)

def _set_log_level_then_assert_python_logger_has_the_level():
    assert opyn.config.get_log_level() == logging.WARN
    opynsim_logger = logging.getLogger(opyn.__name__)
    assert opynsim_logger.level == logging.NOTSET
    opyn.config.set_log_level(logging.DEBUG)
    assert  opynsim_logger.level == logging.DEBUG

def test_set_log_level_makes_associated_python_logger_also_return_new_log_level():
    execute_forked(_set_log_level_then_assert_python_logger_has_the_level)

def test_get_search_paths_contains_expected_defaults():
    search_paths = opyn.config.get_search_paths()

    assert Path("Geometry") in search_paths, "Geometry/ is the standard (legacy) location where OpenSim searches for mesh files"
    assert Path("geometry") in search_paths, "geometry/ is a backwards-compatibility shim for models that were developed in Windows (case-insensitive filesystem) but need to work on Unixes"
    assert Path(".")        in search_paths, "The current directory should also be in there: it's a legacy fallback from OpenSim"

def _prepend_search_path_then_assert_it_was_prepended():
    paths_before = opyn.config.get_search_paths()
    prepended_path = Path("somepath")
    assert len(paths_before) > 0
    assert paths_before[0] != prepended_path
    opyn.config.prepend_search_path(prepended_path)
    paths_after = opyn.config.get_search_paths()
    assert paths_after != paths_before
    assert paths_after[0] == prepended_path

def test_prepend_search_path_prepends_the_search_path():
    execute_forked(_prepend_search_path_then_assert_it_was_prepended)

def _append_search_path_then_assert_it_was_appended():
    paths_before = opyn.config.get_search_paths()
    appended_path = Path("some_appended_path")
    assert len(paths_before) > 0
    assert paths_before[-1] != appended_path
    opyn.config.append_search_path(appended_path)
    paths_after = opyn.config.get_search_paths()
    assert paths_after != paths_before
    assert paths_after[-1] == appended_path

def test_append_search_path_appends_the_search_path():
    execute_forked(_append_search_path_then_assert_it_was_appended)

def _remove_search_path_then_assert_it_was_removed():
    paths_before = opyn.config.get_search_paths()
    path_to_remove = Path("Geometry")  # The test assumes this is always in the default path list

    assert path_to_remove in paths_before
    assert opyn.config.remove_search_path(path_to_remove), "Function should return `True` if something was removed"
    paths_after = opyn.config.get_search_paths()
    assert paths_after != paths_before
    assert str(path_to_remove) not in [str(e) for e in paths_after]  # care: Windows is case-insensitive, so do a string comparison

def test_remove_search_path_removes_the_search_path():
    execute_forked(_remove_search_path_then_assert_it_was_removed)
    assert Path("Geometry") in opyn.config.get_search_paths(), "Sanity check: forking should prevent side-effects"
