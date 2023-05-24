# e.g. /C/Program\ Files/Blender\ Foundation/Blender\ 3.2/blender.exe --python load_dae_file_in_blender.py

import bpy

def reset_blend():
    # Select objects by type
    for o in bpy.context.scene.objects:
        if o.type == 'MESH':
            o.select_set(True)
        else:
            o.select_set(False)

    # Call the operator only once
    bpy.ops.object.delete()

bpy.context.preferences.view.show_splash = False
reset_blend()
imported = bpy.ops.wm.collada_import(filepath="DAE_FILE_PATH")