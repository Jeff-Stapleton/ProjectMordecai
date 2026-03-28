"""US-054 Step 3: Create Widget Blueprints for HUD.

AC-054.7: WBP_CombatHUD (Health, Stamina, Posture bars)
AC-054.9: WBP_EnemyHealthBar (world-space enemy health)
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

EAL.make_directory('/MordecaiCore/UI')

def create_widget_bp(name, parent_class, folder='/MordecaiCore/UI'):
    """Create a Widget Blueprint with the given parent class."""
    path = f'{folder}/{name}'
    if EAL.does_asset_exist(path):
        unreal.log(f'Deleting existing: {path}')
        EAL.delete_asset(path)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', parent_class)

    wb = asset_tools.create_asset(name, folder, unreal.WidgetBlueprint, factory)
    if wb is None:
        unreal.log_error(f'Failed to create {name}')
        return None

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created widget blueprint: {path}')
    return wb

# --- AC-054.7: Combat HUD ---
wbp_hud = create_widget_bp('WBP_CombatHUD', unreal.MordecaiCombatHUDWidget)

# --- Sub-widgets for the HUD (used by BindWidgetOptional) ---
create_widget_bp('WBP_HealthBar', unreal.MordecaiHealthBarWidget)
create_widget_bp('WBP_StaminaBar', unreal.MordecaiStaminaBarWidget)
create_widget_bp('WBP_PostureBar', unreal.MordecaiPostureBarWidget)

# --- AC-054.9: Enemy Health Bar ---
create_widget_bp('WBP_EnemyHealthBar', unreal.MordecaiEnemyHealthBarWidget)

print('SUCCESS: All Widget Blueprints created.')
