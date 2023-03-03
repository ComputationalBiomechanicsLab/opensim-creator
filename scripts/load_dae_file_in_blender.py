import bpy
import time

#    osc::OpenSimApp app;
#    std::filesystem::path model{R"(C:\Users\adamk\OneDrive\Desktop\opensim-creator\resources\models\RajagopalModel\Rajagopal2015.osim)"};
#    auto undoableThing = osc::LoadOsimIntoUndoableModel(model);
#    std::vector<osc::ComponentDecoration> decs;
#    auto opts = osc::CustomDecorationOptions{};
#    opts.setMuscleColoringStyle(osc::MuscleColoringStyle::Activation);
#    opts.setMuscleDecorationStyle(osc::MuscleDecorationStyle::Scone);
#    opts.setMuscleSizingStyle(osc::MuscleSizingStyle::SconePCSA);
#    osc::GenerateModelDecorations(*undoableThing, decs, opts);
#    std::vector<osc::BasicSceneElement> basicDecs;
#    for (osc::ComponentDecoration const& dec : decs)
#    {
#        osc::BasicSceneElement& basic = basicDecs.emplace_back();
#        basic.transform = dec.transform;
#        basic.mesh = dec.mesh;
#        basic.color = dec.color;
#    }
#
#    std::ofstream f{"model.dae"};
#    osc::WriteDecorationsAsDAE(basicDecs, f);
#    return 0;


def reset_blend():
    # Select objects by type
    for o in bpy.context.scene.objects:
        if o.type == 'MESH':
            o.select_set(True)
        else:
            o.select_set(False)

    # Call the operator only once
    bpy.ops.object.delete()

# cd ~/OneDrive/Desktop/opensim-creator/out/build/x64-Release && ~/OneDrive/Desktop/opensim-creator/out/build/x64-Release/osc.exe && cat ~/OneDrive/Desktop/opensim-creator/out/build/x64-Release/model.dae && /C/Program\ Files/Blender\ Foundation/Blender\ 3.2/blender.exe --python ~/OneDrive/Desktop/opensim-creator/scripts/blender_hack.py

#with open("C:\\Users\\adamk\\OneDrive\\Desktop\\opensim-creator\\out\\build\\x64-Release\\model.dae") as f:
#    print(f.read())

# other 

bpy.context.preferences.view.show_splash = False
reset_blend()
imported = bpy.ops.wm.collada_import(filepath="C:\\Users\\adamk\\OneDrive\\Desktop\\opensim-creator\\out\\build\\x64-Release\\model.dae")