"""Check what Mordecai types are available in Python."""
import unreal

names_to_try = [
    'EMordecaiAttackType', 'MordecaiAttackType',
    'MordecaiAttackProfileDataAsset', 'UMordecaiAttackProfileDataAsset',
    'MordecaiEnemyCharacter', 'AMordecaiEnemyCharacter',
    'MordecaiEnemyAIController', 'AMordecaiEnemyAIController',
    'MordecaiCombatHUDWidget', 'UMordecaiCombatHUDWidget',
    'MordecaiHealthBarWidget', 'MordecaiStaminaBarWidget',
    'MordecaiPostureBarWidget', 'MordecaiEnemyHealthBarWidget',
    'MordecaiGA_MeleeAttack', 'MordecaiGA_Dodge',
    'MordecaiGA_Block', 'MordecaiGA_Parry',
    'DataAssetFactory', 'BlueprintFactory', 'WidgetBlueprintFactory',
    'EMordecaiHitShapeType', 'MordecaiHitShapeType',
    'EMordecaiDamageType', 'MordecaiDamageType',
    'EMordecaiInputSlot', 'MordecaiInputSlot',
    'EMordecaiRootedMode', 'MordecaiRootedMode',
    'MordecaiDamageProfile', 'FMordecaiDamageProfile',
    'MordecaiHitShapeParams', 'FMordecaiHitShapeParams',
]

for name in names_to_try:
    has = hasattr(unreal, name)
    if has:
        obj = getattr(unreal, name)
        print(f'YES: {name} -> {type(obj).__name__}')
    else:
        print(f' NO: {name}')

# Also search for anything with "Mordecai" in it
mordecai_attrs = [a for a in dir(unreal) if 'Mordecai' in a or 'mordecai' in a]
print(f'\nAll Mordecai-related attrs ({len(mordecai_attrs)}):')
for a in sorted(mordecai_attrs):
    print(f'  {a}')
