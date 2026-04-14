import opynsim

import pathlib
import pytest

def test_can_construct_blank_ModelSpecification():
    model_specification = opynsim.ModelSpecification()

def test_add_opensim_geometry_directory_throws_if_given_invalid_path():
    with pytest.raises(RuntimeError):
        opynsim.add_opensim_geometry_directory(pathlib.Path("/this/doesnt/exist"))

def test_import_osim_file_throws_if_given_invalid_path():
    with pytest.raises(Exception):
        opynsim.import_osim_file("/this/doesnt/exist")

def test_import_osim_file_does_not_produce_mesh_missing_warnings():
    # Sanity check: by default, OpenSim finalizes properties when an
    # OpenSim::Model is loaded, but OPynSim should only finalize the
    # properties once `compile` is called.
    #
    # Notably, mesh file resolution must happen during `compile`,
    # because that's where the caller can specify additional include
    # directories. They don't do this
    pass
