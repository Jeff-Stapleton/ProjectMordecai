"""US-057 Step 4: Create spell HUD sub-widget Blueprints.

AC-057.9: WBP_CombatHUD needs SP bar, status effect indicator bar,
          and 4 spell cooldown indicators.

Creates the widget Blueprints that the C++ BindWidgetOptional properties expect:
- WBP_SpellPointsBar (binds to SpellPointsBar slot)
- WBP_StatusEffectBar (binds to StatusEffectBar slot)
- WBP_SpellCooldownA (binds to SpellCooldownA slot)
- WBP_SpellCooldownB (binds to SpellCooldownB slot)
- WBP_StatusEffectIndicator (template for dynamic status indicators)
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

EAL.make_directory('/MordecaiCore/UI')


def create_widget_bp(name, parent_class, folder='/MordecaiCore/UI'):
    """Create a Widget Blueprint with the given parent class."""
    path = f'{folder}/{name}'
    if EAL.does_asset_exist(path):
        unreal.log(f'Already exists: {path}')
        return unreal.load_asset(path)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', parent_class)

    wb = asset_tools.create_asset(name, folder, unreal.WidgetBlueprint, factory)
    if wb is None:
        unreal.log_error(f'Failed to create {name}')
        return None

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created widget blueprint: {path}')
    return wb


# --- AC-057.9: Spell HUD Sub-Widgets ---
create_widget_bp('WBP_SpellPointsBar', unreal.MordecaiSpellPointsBarWidget)
create_widget_bp('WBP_StatusEffectBar', unreal.MordecaiStatusEffectBarWidget)
create_widget_bp('WBP_SpellCooldownA', unreal.MordecaiSpellCooldownWidget)
create_widget_bp('WBP_SpellCooldownB', unreal.MordecaiSpellCooldownWidget)
create_widget_bp('WBP_StatusEffectIndicator', unreal.MordecaiStatusEffectIndicatorWidget)

# Also ensure the combo/feedback widgets exist (from US-061)
try:
    create_widget_bp('WBP_ComboCounter', unreal.MordecaiComboCounterWidget)
except Exception as e:
    unreal.log_warning(f'Could not create WBP_ComboCounter: {e}')

try:
    create_widget_bp('WBP_CombatFeedback', unreal.MordecaiCombatFeedbackWidget)
except Exception as e:
    unreal.log_warning(f'Could not create WBP_CombatFeedback: {e}')

try:
    create_widget_bp('WBP_KillCounter', unreal.MordecaiKillCounterWidget)
except Exception as e:
    unreal.log_warning(f'Could not create WBP_KillCounter: {e}')

unreal.log('US-057 Step 4 complete: All spell HUD widget BPs created.')
print('SUCCESS: Spell HUD widget blueprints created.')
