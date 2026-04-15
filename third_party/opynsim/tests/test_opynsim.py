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
