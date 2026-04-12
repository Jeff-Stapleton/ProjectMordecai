# Fix PawnData_Mordecai to use MordecaiCameraMode_Diorama
import unreal

EAL = unreal.EditorAssetLibrary

pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if pd:
    # Check current
    cam = pd.get_editor_property('default_camera_mode')
    print(f'Current DefaultCameraMode: {cam.get_name() if cam else "None"}')

    # Set to MordecaiCameraMode_Diorama
    pd.set_editor_property('default_camera_mode', unreal.MordecaiCameraMode_Diorama)
    EAL.save_asset('/MordecaiCore/System/PawnData_Mordecai', only_if_is_dirty=False)

    # Verify
    cam2 = pd.get_editor_property('default_camera_mode')
    print(f'New DefaultCameraMode: {cam2.get_name() if cam2 else "None"}')
    print('Saved. Stop and restart PIE to pick up the change.')
else:
    print('FATAL: PawnData_Mordecai not found')
