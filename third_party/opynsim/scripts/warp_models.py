#!/usr/bin/env python3

import opynsim as opyn
import opynsim.solvers
from pathlib import Path

root_path = Path("/home/adam/Desktop/ModelWarper_API_test/")
model_path = root_path / "msk_morph/template_model_and_settings/StationDefinedTemplateModel_HipJoints.osim"
participant_data_dir = root_path / "participant_data"

template_spec = opyn.read_osim(model_path)
for p_dir in participant_data_dir.iterdir():
    if not p_dir.is_dir():
        continue
    participant_warper = opyn.solvers.ModelWarper.from_xml(p_dir / "warping_files/SettingsModelWarper_StationDefinedTemplateModel_HipJoints.xml")
    warped_spec = participant_warper.warp(template_spec)
    warped_spec.root_directory = p_dir
    warped_spec.flush_in_memory_resources_to("morped_geometry")
    # warped_spec.to_osim(p_dir / "StationDefinedMorphedModel_HipJoints_WithSDFs.osim")  # If desired
    warped_spec.bake_station_defined_frames()
    warped_spec.to_osim(p_dir / "StationDefinedMorphedModel_HipJoints.osim")

