import opynsim

from pathlib import Path
import pytest

def test_can_default_construct_model_specification():
    model_specification = opynsim.ModelSpecification()

def test_model_initial_state_state_defaults_to_instance():
    model = opynsim.ModelSpecification().compile()
    assert model.initial_state().stage == opynsim.STAGE_INSTANCE

def test_model_initial_state_realize_to_realizes_model_to_state():
    model = opynsim.ModelSpecification().compile()
    assert model.initial_state(realized_to=opynsim.STAGE_ACCELERATION).stage == opynsim.STAGE_ACCELERATION

def test_can_default_construct_data_frame():
    data_frame = opynsim.DataFrame()
    assert "shape: (0, 0)" in repr(data_frame)
    assert "shape: (0, 0)" in str(data_frame)

def test_data_frame_from_arrow_works_on_pandas_data_frame():
    import pandas

    pandas_df = pandas.DataFrame({
        "col_a": [1.0, 2.0, 3.0],
        "col_b": [4.0, 5.0, 6.0],
    })

    opynsim_df = opynsim.DataFrame.from_arrow(pandas_df)
    assert opynsim_df.shape == pandas_df.shape

def test_data_frame_from_arrow_works_on_polars_data_frame():
    import polars

    polars_df = polars.DataFrame({
        "col_a": [1.0, 2.0, 3.0],
        "col_b": [4.0, 5.0, 6.0],
    })

    opynsim_df = opynsim.DataFrame.from_arrow(polars_df)
    assert opynsim_df.shape == polars_df.shape

def test_data_frame_from_arrow_works_on_pyarrow_table():
    import pyarrow as pa

    pa_schema = pa.schema([
        pa.field("col_1", pa.float64()),
        pa.field("col_2", pa.float64()),
    ])
    pa_table = pa.Table.from_pydict({
        "col_1": [100.0, 200.0],
        "col_2": [0.001, 0.002]
    }, schema=pa_schema)

    opynsim_df = opynsim.DataFrame.from_arrow(pa_table)

    assert opynsim_df.shape == pa_table.shape

def test_data_frame_has__arrow_c_schema():
    data_frame = opynsim.DataFrame()
    assert hasattr(data_frame, "__arrow_c_stream__")
    assert type(data_frame.__arrow_c_stream__()).__name__ == "PyCapsule"

def test_data_frame_arrow_api_is_compatible_with_polars_constructor():
    import polars
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/double_pendulum_run.sto")
    polars_df = polars.DataFrame(df)
    assert polars_df.shape == df.shape

def test_data_frame_arrow_api_is_compatible_with_polars_from_arrow():
    import polars
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/double_pendulum_run.sto")
    polars_df = polars.from_arrow(df)
    assert polars_df.shape == df.shape

def test_data_frame_arrow_api_is_compatible_with_pyarrow():
    import pyarrow  # note `pyarrow` is a runtime dependency of `pandas` when importing Arrow dataframes

    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/double_pendulum_run.sto")
    pyarrow_table = pyarrow.table(df)
    assert pyarrow_table.shape == df.shape

def test_data_frame_arrow_api_is_compatible_with_pandas_from_arrow():
    import pandas
    import pyarrow  # runtime dependency

    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/double_pendulum_run.sto")
    pandas_df = pandas.DataFrame.from_arrow(df)
    assert pandas_df.shape == df.shape

def test_data_frame_to_pandas_works():
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/sto/double_pendulum_run.sto")
    pandas_df = df.to_pandas()

    assert pandas_df.shape == df.shape

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

def test_read_sto_column_to_state_variable_mappings_are_as_expected():
    model = opynsim.read_osim(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum.osim").compile()
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum_trajectory.sto")
    expected = {
        "/jointset/pin/pin_coord_0/value": "/jointset/pin/pin_coord_0/value",
        "/jointset/pin/pin_coord_0/speed": "/jointset/pin/pin_coord_0/speed",
    }

    assert model.column_to_state_variable_mappings(df) == expected

def test_read_sto_is_compatible_with_rotational_columns_in_model():
    model = opynsim.read_osim(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum.osim").compile()
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum_trajectory.sto")

    assert model.rotational_columns_in(df) == {"/jointset/pin/pin_coord_0/value", "/jointset/pin/pin_coord_0/speed"}

def test_read_sto_is_compatible_with_convert_data_frame_to_radians():
    model = opynsim.read_osim(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum.osim").compile()
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum_trajectory.sto")
    converted = model.convert_data_frame_to_radians(df)

    assert df.shape == converted.shape
    # TODO: also test values, but this requires a reader API in Python

def test_read_sto_is_compatible_with_states_from_data_frame():
    model = opynsim.read_osim(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum.osim").compile()
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum_trajectory.sto")

    states = model.states_from_data_frame(df)

    # By default, the states should all be realized to STAGE_INSTANCE
    for state in states:
        assert state.stage == opynsim.STAGE_INSTANCE

    # Indexed access (`__len__`, `__getitem__`) should work like a list
    # and the yielded `ModelState`s should be compatible with the existing
    # state API.
    assert len(states) == 201
    for i in range(201):
        model.realize(states[i], opynsim.STAGE_POSITION)
        model.get_output_value(states[i], "/bodyset/head[position]")

    # Negative indices behave identically to Python
    assert states[200] == states[-1]

    # Out-of-bounds access should raise an `IndexError` (compatible with
    # the rest of the Python API).
    with pytest.raises(IndexError):
        _ = states[201]  # OOB

    # Out-of-bounds access via a negative index should raise an `IndexError`
    # (compatible with the rest of the Python API).
    with pytest.raises(IndexError):
        _ = states[-202]  # OOB

    # Foreach (iterator) loops should also work
    for state in states:
        model.realize(state, opynsim.STAGE_ACCELERATION)
        model.get_output_value(state, "/bodyset/head[angular_velocity]")

    # Reverse iteration should also work
    assert list(reversed(states))[0] == states[-1]

    # ... and list comprehensions...
    positions = [model.get_output_value(s, "/bodyset/head[position]") for s in states]
    assert len(positions)

    # ... and list slicing...
    sliced = states[5:10]
    assert isinstance(sliced, opynsim.ModelStates)
    assert isinstance(sliced.to_list(), list)
    assert sliced.to_list() == [states[5], states[6], states[7], states[8], states[9]]

    # ... and list slicing with a step size...
    step_sliced = states[5:10:2]
    assert step_sliced.to_list() == [states[5], states[7], states[9]]

    # ... and slicing with unusual indices
    negative_sliced = states[-10:-1:3]
    assert negative_sliced.to_list() == [states[-10], states[-7], states[-4]]

def test_states_from_data_frame_realized_to_works_as_intended():
    model = opynsim.read_osim(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum.osim").compile()
    df = opynsim.read_sto(Path(__file__).resolve().parent / "../libopynsim/tests/resources/pendulum/pendulum_trajectory.sto")

    states = model.states_from_data_frame(df, realized_to=opynsim.STAGE_ACCELERATION)
    for state in states:
        assert state.stage == opynsim.STAGE_ACCELERATION

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
    import numpy as np

    mesh = opynsim.read_vtp(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/triangle.vtp")
    vertices = mesh.vertices
    faces = mesh.faces

    assert vertices.dtype == np.float32
    assert faces.dtype == np.int32
    assert vertices.shape == (3, 3)
    assert np.array_equal(vertices, np.array([[-1.0, -1.0, 0.0], [1.0, -1.0, 0.0], [0.0, 1.0, 0.0]]))
    assert np.array_equal(faces, np.array([0, 1, 2]))

def test_read_obj_can_read_a_basic_obj_file():
    import numpy as np

    mesh = opynsim.read_obj(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/triangle.obj")
    vertices = mesh.vertices
    faces = mesh.faces

    assert vertices.dtype == np.float32
    assert faces.dtype == np.int32
    assert vertices.shape == (3, 3)
    assert np.array_equal(vertices, np.array([[-1.0, -1.0, 0.0], [1.0, -1.0, 0.0], [0.0, 1.0, 0.0]]))
    assert np.array_equal(faces, np.array([0, 1, 2]))

def test_read_stl_can_read_a_basic_stl_file():
    import numpy as np

    mesh = opynsim.read_stl(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/triangle.stl")
    vertices = mesh.vertices
    faces = mesh.faces

    assert vertices.dtype == np.float32
    assert faces.dtype == np.int32
    assert vertices.shape == (3, 3)
    assert np.array_equal(vertices, np.array([[-1.0, -1.0, 0.0], [1.0, -1.0, 0.0], [0.0, 1.0, 0.0]]))
    assert np.array_equal(faces, np.array([0, 1, 2]))

def test_read_png_can_read_minimal_png_file():
    import numpy as np

    png = opynsim.read_png(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/minimal.png")
    pixels = png.pixels_rgba32()

    assert pixels.shape == (1, 1, 4)
    assert np.array_equal(pixels[0, 0], np.array([255, 255, 255, 255]))

def test_read_jpeg_can_read_minimal_jpeg_file():
    import numpy as np

    jpeg = opynsim.read_jpeg(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/minimal.jpeg")
    pixels = jpeg.pixels_rgba32()

    assert pixels.shape == (1, 1, 4)
    assert np.array_equal(pixels[0, 0], np.array([255, 255, 255, 255]))

def test_read_jpg_alias_also_works():
    import numpy as np

    jpeg = opynsim.read_jpg(Path(__file__).resolve().parent / "../libopynsim/tests/resources/Documents/minimal.jpeg")
    pixels = jpeg.pixels_rgba32()

    assert pixels.shape == (1, 1, 4)
    assert np.array_equal(pixels[0, 0], np.array([255, 255, 255, 255]))
