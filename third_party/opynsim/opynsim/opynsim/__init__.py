import opynsim._opynsim_native

from opynsim._opynsim_native import set_logging_level
from opynsim._opynsim_native import ModelStateStage

# Imports an `.osim` file into an `opynsim.ModelSpecification`.
#
# Throws an exception if the path is invalid, unreadable, or contains
# invalid OpenSim model markup.
def import_osim_file(osim_file_path):
    # Pre-convert string-like objects to strings (e.g. `pathlib.Path`s)
    # before passing it into the native API, which only accepts strings
    osim_file_path_string = str(osim_file_path)

    return opynsim._opynsim_native.import_osim_file(osim_file_path_string)

def add_geometry_directory(directory_path):
    # Pre-convert string-like objects to strings (e.g. `pathlib.Path`s)
    # before passing it into the native API, which only accepts strings
    directory_path_string = str(directory_path)

    opynsim._opynsim_native.add_geometry_directory(directory_path_string)
