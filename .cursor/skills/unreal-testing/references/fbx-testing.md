# FBX Test Builder — Full Reference

Enable the **FBXAutomationTestBuilder** plugin (`Edit > Plugins > Testing`). Access via `Window > Automation Tools > FBX Test Builder`.

## Concept

Create **Test Plans** (stored as JSON) for individual FBX files. Each plan defines an **Action** to perform and **Expected Results** to check.

## Actions

| Action | Description | Prerequisites |
|--------|-------------|---------------|
| **Import** | Import FBX, verify results | None |
| **Reimport** | Reimport previously imported FBX | Must follow an Import plan (assets not deleted) |
| **Add LOD** | Add LOD from `_lod0X` variant | Must follow Import; FBX named `*_lod00`; `*_lod0X` files exist |
| **Reimport LOD** | Reimport previously imported LODs | Previous Import or Add LOD |
| **Import Reload** | Import → Save → Unload → Reload → Verify | None |

## Expected Result Presets

| Check Type | Required Data |
|------------|---------------|
| Error Number | int (count) |
| Warning Number | int (count) |
| Created Static Mesh Number | int |
| Created Skeletal Mesh Number | int |
| Materials Created Number | int |
| Material Slot Imported Name | int (index) + string (name) |
| Vertex Number | int (total across all LODs) |
| LOD Number | int |
| Vertex Number LOD | int (LOD index) + int (vertex count) |
| Mesh Materials Number | int |
| Mesh LOD Section Number | int (LOD) + int (section count) |
| Mesh LOD Section Vertex Number | int (LOD) + int (section) + int (vertices) |
| Mesh LOD Section Triangle Number | int (LOD) + int (section) + int (triangles) |
| Mesh LOD Section Material Name | int (LOD) + int (section) + string (name) |
| Mesh LOD Section Material Index | int (LOD) + int (section) + int (material index) |
| Mesh LOD Section Material Imported Name | int (LOD) + int (section) + string (name) |
| LOD UV Channel Number | int (LOD) + int (UV count) |
| Bone Number | int |
| Bone Position | int (index) + float×3 (XYZ) + optional float (tolerance) |
| Animation Frame Number | int |
| Animation Length | float |

## Workflow

1. Copy FBX to `[UE Root]/Engine/Content/` (auto-populates dropdown)
2. Open FBX Test Builder, select FBX file
3. Create new plan or edit existing
4. Set: Plan Name, Action, Expected Results, Import Options
5. Click **Save JSON**
6. Tests appear under `Engine/Import/FBX/[FBXName]` in Session Frontend
7. Run and review results in Automation Test Results panel
