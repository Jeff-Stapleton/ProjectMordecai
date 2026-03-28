"""US-054 Step 2: Create BP_MordecaiEnemy_Frontliner Blueprint.

AC-054.1: Blueprint parented to AMordecaiEnemyCharacter with AMordecaiEnemyAIController
AC-054.2: Stats: MaxHealth=200, MaxPosture=100, MoveSpeed=400
           AI params: AggroRange=900, AttackRange=200, LeashRange=1800
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

EAL.make_directory('/MordecaiCore/Enemies')

bp_path = '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner'
if EAL.does_asset_exist(bp_path):
    unreal.log(f'Deleting existing: {bp_path}')
    EAL.delete_asset(bp_path)

# Create Blueprint with MordecaiEnemyCharacter as parent
factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.MordecaiEnemyCharacter)

bp = asset_tools.create_asset(
    'BP_MordecaiEnemy_Frontliner',
    '/MordecaiCore/Enemies',
    unreal.Blueprint,
    factory
)

if bp is None:
    unreal.log_error('Failed to create BP_MordecaiEnemy_Frontliner')
    print('ERROR: Failed to create enemy blueprint')
else:
    # Get CDO via load_class
    gen_class = unreal.load_class(None, bp_path + '.BP_MordecaiEnemy_Frontliner_C')
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if cdo:
            # Set enemy character stats (AC-054.2)
            cdo.set_editor_property('default_max_health', 200.0)
            cdo.set_editor_property('default_max_posture', 100.0)
            cdo.set_editor_property('default_move_speed', 400.0)

            # Set AI controller class
            cdo.set_editor_property('ai_controller_class', unreal.MordecaiEnemyAIController)

            unreal.log('Set enemy stats: HP=200, Posture=100, Speed=400, AI=MordecaiEnemyAIController')
            unreal.log('AI ranges use C++ defaults: Aggro=900, Attack=200, Leash=1800')
        else:
            unreal.log_warning('Could not get CDO')
    else:
        unreal.log_warning('Could not load generated class')

    EAL.save_asset(bp_path, only_if_is_dirty=False)
    unreal.log(f'Created and saved: {bp_path}')
    print('SUCCESS: BP_MordecaiEnemy_Frontliner created.')
