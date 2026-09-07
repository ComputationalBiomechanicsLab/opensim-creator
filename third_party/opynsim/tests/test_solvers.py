import opynsim as opyn
from opynsim.solvers import ModelWarper

from pathlib import Path
import inspect
import tempfile
import pytest

def test_solvers_module_contains_the_model_warper_solver_class():
    assert inspect.isclass(ModelWarper)

def test_model_warper_has_default_constructor():
    model_warper = ModelWarper()  # shouldn't throw

def test_model_warper_from_xml_throws_when_given_invalid_path():
    with pytest.raises(Exception):
        ModelWarper.from_xml("does/not/exist")

def test_model_warper_from_xml_works_when_given_a_valid_scaling_document():
    # Shouldn't throw
    ModelWarper.from_xml(Path(__file__).resolve().parent.parent / "libopynsim/tests/resources/Documents/model_warper/scaling-document.xml")

def test_model_warper_can_warp_an_example():
    scaling_document_path = Path(__file__).resolve().parent.parent / "libopynsim/tests/resources/Documents/model_warper/scaling-document.xml"
    model_path = Path(__file__).resolve().parent.parent / "libopynsim/tests/resources/Documents/model_warper/make-a-leg.osim"

    model_warper = ModelWarper.from_xml(scaling_document_path)
    model_specification = opyn.read_osim(model_path)
    warped_model_specification = model_warper.warp(model_specification)
    assert isinstance(warped_model_specification, opyn.ModelSpecification)

    # Ensure both are compile-able
    model = model_specification.compile()
    model_state = model.initial_state(realized_to=opyn.STAGE_REPORT)
    warped_model = warped_model_specification.compile()
    warped_model_state = warped_model.initial_state(realized_to=opyn.STAGE_REPORT)

    # Perform some basic checks: it is known that this particular warping pipeline
    # makes the model smaller/shorter (i.e. the tibia position is less-negative in Y).
    assert warped_model.coordinates == model.coordinates
    assert warped_model.outputs == model.outputs
    original_tibia_y = -model.get_output_value(model_state, "/bodyset/tibia_r[position]")[1]
    warped_tibia_y = -warped_model.get_output_value(warped_model_state, "/bodyset/tibia_r[position]")[1]
    assert warped_tibia_y < (original_tibia_y-0.05)

    # The `ModelWarper` pipeline that produced this model contains in-memory
    # resources, which prevents `to_osim` from working.
    with pytest.raises(RuntimeError):
        warped_model_specification.to_osim()

    # ... so the caller could just flush them to an absolute path, if they're feeling lazy...
    with tempfile.TemporaryDirectory() as temporary_directory:
        warped_model_specification.flush_in_memory_resources_to(temporary_directory)
        assert Path(temporary_directory) / "femur_r_geom_1.obj", "ModelWarper didn't write the warped meshes?"

    # ... And then it should be serialize-able
    assert "<Mesh" in warped_model_specification.to_osim()
