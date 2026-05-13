import opynsim

from pathlib import Path
import pytest

def test_can_construct_blank_ModelSpecification():
    model_specification = opynsim.ModelSpecification()

def test_read_osim_throws_if_given_invalid_path():
    with pytest.raises(Exception):
        opynsim.read_osim("/this/doesnt/exist")

def test_read_sto_throws_if_given_invalid_path():
    with pytest.raises(Exception):
        opynsim.read_sto("/this/doesnt/exist")

def test_read_sto_can_read_and_print_a_basic_sto_file():
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/two_data_columns.sto")

    repr_printed = repr(df)
    stringified = str(df)

    assert repr_printed == stringified
    assert repr_printed == """shape: (2, 3)
| time | column1 | column2 |
|:-----|:--------|:--------|
| 0.0  | 2.0     | 3.0     |
| 1.0  | 4.0     | 6.0     |
"""
