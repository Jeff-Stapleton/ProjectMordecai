"""Check enum value names."""
import unreal

# Check MordecaiAttackType enum values
print('MordecaiAttackType values:')
for attr in dir(unreal.MordecaiAttackType):
    if not attr.startswith('_'):
        print(f'  {attr}')

print('\nMordecaiHitShapeType values:')
for attr in dir(unreal.MordecaiHitShapeType):
    if not attr.startswith('_'):
        print(f'  {attr}')

print('\nMordecaiDamageType values:')
for attr in dir(unreal.MordecaiDamageType):
    if not attr.startswith('_'):
        print(f'  {attr}')

print('\nMordecaiInputSlot values:')
for attr in dir(unreal.MordecaiInputSlot):
    if not attr.startswith('_'):
        print(f'  {attr}')

print('\nMordecaiRootedMode values:')
for attr in dir(unreal.MordecaiRootedMode):
    if not attr.startswith('_'):
        print(f'  {attr}')

# Check DataAssetFactory properties
print('\nDataAssetFactory props:')
f = unreal.DataAssetFactory()
for attr in dir(f):
    if not attr.startswith('_') and not attr.startswith('get_') and not attr.startswith('set_'):
        try:
            val = getattr(f, attr)
            if not callable(val):
                print(f'  {attr} = {val}')
        except:
            pass
