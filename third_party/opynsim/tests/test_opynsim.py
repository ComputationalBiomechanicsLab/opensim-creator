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

def test_read_sto_attrs_contains_expected_basic_headers():
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/two_data_columns.sto")
    expected_attrs = {"meta1": "a", "meta2": "b"}

    assert df.attrs == expected_attrs

def test_read_sto_attrs_contains_in_degrees_when_yes_in_sto_header():
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/in_degrees.sto")
    expected_attrs = {"inDegrees": "yes"}

    assert df.attrs == expected_attrs

def test_read_sto_attrs_contains_in_degrees_when_no_in_sto_header():
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/in_radians.sto")
    expected_attrs = {"inDegrees": "no"}

    assert df.attrs == expected_attrs

def test_read_sto_attrs_normalizes_in_degrees_to_yes_when_passed_legacy_y_entry():
    # Legacy STO file parsers accepted both "yes" and "y" (case-insensitive) for the
    # `inDegrees` key. This should be normalized by the backend so that downstream
    # code doesn't need to be aware of this legacy behavior.

    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/in_degrees_legacy_y.sto")
    expected_attrs = {"inDegrees": "yes"}

    assert df.attrs == expected_attrs

def test_read_sto_attrs_normalizes_in_degrees_to_yes_when_given_case_insensitive_format():
    # Legacy STO file parsers handled the `inDegrees` key in a case-insensitive way. Meaning
    # `inDegrees=YES`, `inDegrees=yes`, and `inDegrees=YeS` all behave the same. Similar story
    # for `inDegrees=y` and `inDegrees=Y`: all should be normalized to `inDegrees=yes` so that
    # downstream code doesn't need to be aware of this legacy behavior.

    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/in_degrees_case_insensitive_yes.sto")
    expected_attrs = {"inDegrees": "yes"}

    assert df.attrs == expected_attrs

def test_read_sto_handles_weird_angles_in_degrees_string_match():
    # Legacy STO file parsers had an edge-case where they'd handle version=0 files
    # with a line containing "Angles are in degrees." as equivalent to `inDegrees=yes`.

    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/in_degrees_angles_header.sto")
    expected_attrs = {"header": "Angles are in degrees.", "inDegrees": "yes"}

    assert df.attrs == expected_attrs

def test_read_sto_handles_weird_angles_in_radians_string_match():
    # Legacy STO file parsers had an edge-case where they'd handle version=0 files
    # with a line containing "Angles are in degrees." as equivalent to `inDegrees=yes`.

    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/in_radians_angles_header.sto")
    expected_attrs = {"header": "Angles are in radians.", "inDegrees": "no"}

    assert df.attrs == expected_attrs

def test_read_sto_doesnt_add_in_degrees_when_not_mentioned():
    # With STO files, the absence of an inDegrees header implies the data is in radians.

    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/in_radians_implicit.sto")
    expected_attrs = {}

    assert df.attrs == expected_attrs

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
