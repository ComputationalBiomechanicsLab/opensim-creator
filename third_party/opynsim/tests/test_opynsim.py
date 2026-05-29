import opynsim

import numpy as np
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

def test_read_mot_can_read_and_print_a_basic_mot_file():
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/one_data_column.mot")

    repr_printed = repr(df)
    stringified = str(df)

    assert repr_printed == stringified
    assert repr_printed == """shape: (1, 2)
| time | column1 |
|:-----|:--------|
| 0.0  | 2.0     |
"""

def test_read_trc_can_read_and_print_a_minimal_trc_file():
    df = opynsim.read_trc(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/minimal.trc")
    repr_printed = repr(df)
    stringified = str(df)

    assert repr_printed == stringified
    assert repr_printed == """shape: (1, 4)
| time | Marker1_x | Marker1_y | Marker1_z |
|:-----|:----------|:----------|:----------|
| 1.0  | 1.0       | 2.0       | 3.0       |
"""

def test_read_csv_can_read_and_print_a_basic_csv_file():
    df = opynsim.read_csv(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/two_rows.csv")
    repr_printed = repr(df)
    stringified = str(df)

    assert repr_printed == stringified
    assert repr_printed == """shape: (2, 4)
| time | x   | y   | z   |
|:-----|:----|:----|:----|
| 1.0  | 1.0 | 2.0 | 3.0 |
| 2.0  | 2.0 | 4.0 | 6.0 |
"""

def test_read_vtp_can_read_a_basic_vtp_file():
    mesh = opynsim.read_vtp(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/triangle.vtp")
    vertices = mesh.vertices
    faces = mesh.faces

    assert vertices.dtype == np.float32
    assert faces.dtype == np.int32
    assert vertices.shape == (3, 3)
    assert np.array_equal(vertices, np.array([[-1.0, -1.0, 0.0], [1.0, -1.0, 0.0], [0.0, 1.0, 0.0]]))
    assert np.array_equal(faces, np.array([0, 1, 2]))

def test_read_obj_can_read_a_basic_obj_file():
    mesh = opynsim.read_obj(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/triangle.obj")
    vertices = mesh.vertices
    faces = mesh.faces

    assert vertices.dtype == np.float32
    assert faces.dtype == np.int32
    assert vertices.shape == (3, 3)
    assert np.array_equal(vertices, np.array([[-1.0, -1.0, 0.0], [1.0, -1.0, 0.0], [0.0, 1.0, 0.0]]))
    assert np.array_equal(faces, np.array([0, 1, 2]))

def test_read_stl_can_read_a_basic_stl_file():
    mesh = opynsim.read_stl(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/triangle.stl")
    vertices = mesh.vertices
    faces = mesh.faces

    assert vertices.dtype == np.float32
    assert faces.dtype == np.int32
    assert vertices.shape == (3, 3)
    assert np.array_equal(vertices, np.array([[-1.0, -1.0, 0.0], [1.0, -1.0, 0.0], [0.0, 1.0, 0.0]]))
    assert np.array_equal(faces, np.array([0, 1, 2]))

def test_read_png_can_read_minimal_png_file():
    png = opynsim.read_png(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/minimal.png")
    pixels = png.pixels_rgba32()

    assert pixels.shape == (1, 1, 4)
    assert np.array_equal(pixels[0, 0], np.array([255, 255, 255, 255]))

def test_read_jpeg_can_read_minimal_jpeg_file():
    jpeg = opynsim.read_jpeg(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/minimal.jpeg")
    pixels = jpeg.pixels_rgba32()

    assert pixels.shape == (1, 1, 4)
    assert np.array_equal(pixels[0, 0], np.array([255, 255, 255, 255]))

def test_read_jpg_alias_also_works():
    jpeg = opynsim.read_jpg(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/minimal.jpeg")
    pixels = jpeg.pixels_rgba32()

    assert pixels.shape == (1, 1, 4)
    assert np.array_equal(pixels[0, 0], np.array([255, 255, 255, 255]))
