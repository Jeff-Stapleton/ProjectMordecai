# Test how to create FKey in UE5 Python
import unreal

mapping = unreal.EnhancedActionKeyMapping()
key = mapping.get_editor_property('key')
print(f'Key type: {type(key)}')
print(f'Key value: {key}')

# Try setting key via string
try:
    mapping.set_editor_property('key', unreal.Key('W'))
    print('Method 1 worked: unreal.Key("W")')
except Exception as e:
    print(f'Method 1 failed: {e}')

# Try InputChord
try:
    mapping.set_editor_property('key', unreal.InputChord(key_name='W'))
    print('Method 2 worked: unreal.InputChord')
except Exception as e:
    print(f'Method 2 failed: {e}')

# Try via name
try:
    key_obj = unreal.Key()
    key_obj.set_editor_property('key_name', 'W')
    mapping.set_editor_property('key', key_obj)
    print('Method 3 worked: Key().set_editor_property')
except Exception as e:
    print(f'Method 3 failed: {e}')

# Check available Key methods
print(f'Key dir: {[x for x in dir(unreal.Key) if not x.startswith("_")]}')
