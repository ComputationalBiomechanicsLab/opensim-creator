import opynsim

import pytest

def test_can_construct_blank_ModelSpecification():
    model_specification = opynsim.ModelSpecification()

def test_import_osim_file_throws_if_given_invalid_path():
    with pytest.raises(RuntimeError):
        opynsim.import_osim_file("/this/doesnt/exist")
