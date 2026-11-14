"""Offline contract tests for the ready-only US-120 source manifest.

The private B2 catalog remains authoritative.  The committed manifest is its deterministic,
credential-free import allowlist; raw FBX/PNG files are never part of the repository.
"""

import json
import os
import re
import subprocess
import unittest


SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MANIFEST = os.path.join(SCRIPTS, "us120_vw_source_manifest.json")
IMPORTER = os.path.join(SCRIPTS, "us120_import_vw_tiles.py")
PROJECT_ROOT = os.path.dirname(SCRIPTS)

READY_IDS = (
    "SM_VW_FeltCliff_Straight_01",
    "SM_VW_FeltCliff_Straight_02",
    "SM_VW_FeltCliff_Straight_03",
    "SM_VW_FeltCliff_OuterCorner_01",
    "SM_VW_FeltCliff_InnerCorner_01",
    "SM_VW_FeltCliff_EndCap_01",
    "SM_VW_Pit_Edge_01",
    "SM_VW_Pit_OuterCorner_01",
    "SM_VW_Pit_InnerCorner_01",
    "SM_VW_Pond_Edge_01",
    "SM_VW_Pond_OuterCorner_01",
    "SM_VW_Pond_InnerCorner_01",
    "SM_VW_Pond_Edge_Deep_01",
    "SM_VW_Pond_OuterCorner_Deep_01",
    "SM_VW_Pond_InnerCorner_Deep_01",
    "SM_VW_FeltCliff_Ramp_2T_01",
    "SM_VW_FeltCliff_Ramp_3T_01",
)

CATALOG_HULL_COUNTS = {
    "SM_VW_FeltCliff_Straight_01": 1,
    "SM_VW_FeltCliff_Straight_02": 1,
    "SM_VW_FeltCliff_Straight_03": 1,
    "SM_VW_FeltCliff_OuterCorner_01": 1,
    "SM_VW_FeltCliff_InnerCorner_01": 2,
    "SM_VW_FeltCliff_EndCap_01": 1,
    "SM_VW_Pit_Edge_01": 1,
    "SM_VW_Pit_OuterCorner_01": 1,
    "SM_VW_Pit_InnerCorner_01": 2,
    "SM_VW_Pond_Edge_01": 2,
    "SM_VW_Pond_OuterCorner_01": 5,
    "SM_VW_Pond_InnerCorner_01": 6,
    "SM_VW_Pond_Edge_Deep_01": 3,
    "SM_VW_Pond_OuterCorner_Deep_01": 6,
    "SM_VW_Pond_InnerCorner_Deep_01": 7,
    "SM_VW_FeltCliff_Ramp_2T_01": 1,
    "SM_VW_FeltCliff_Ramp_3T_01": 1,
}

SHA1 = re.compile(r"^[0-9a-f]{40}$")
SHA256 = re.compile(r"^[0-9a-f]{64}$")


def load_manifest():
    with open(MANIFEST, encoding="utf-8") as stream:
        return json.load(stream)


class TestUs120SourceManifest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = load_manifest()
        cls.meshes = cls.data["meshes"]
        cls.textures = cls.data["textures"]

    def test_exactly_the_17_ready_catalog_meshes_are_allowlisted(self):
        self.assertEqual(tuple(mesh["id"] for mesh in self.meshes), READY_IDS)
        self.assertEqual(len(self.meshes), 17)
        self.assertEqual({mesh["status"] for mesh in self.meshes}, {"ready"})
        self.assertEqual(self.data["source"]["only_import_status"], "ready")

    def test_master_material_has_cookable_default_textures(self):
        defaults = self.data["import_contract"]["master_material_default_textures"]
        self.assertEqual(
            defaults,
            {
                "BaseColorTexture": "/Engine/EngineResources/DefaultTexture",
                "NormalTexture": "/Engine/EngineMaterials/DefaultNormal",
                "ORMTexture": "/Engine/EngineResources/DefaultTexture",
            },
        )
        self.assertFalse(any("T_VW_" in value for value in defaults.values()))
        with open(IMPORTER, encoding="utf-8") as stream:
            importer = stream.read()
        self.assertIn(
            'set_default_texture(base, "BaseColorTexture", textures, defaults)',
            importer,
        )
        self.assertIn(
            'set_default_texture(normal, "NormalTexture", textures, defaults)',
            importer,
        )
        self.assertIn(
            'set_default_texture(orm, "ORMTexture", textures, defaults)',
            importer,
        )
        self.assertNotIn('"HeightTexture"', importer)

    def test_every_source_key_and_checksum_is_complete_and_unique(self):
        sources = [entry["source"] for entry in (*self.meshes, *self.textures)]
        self.assertEqual(len(sources), 29)
        self.assertEqual(len({source["key"] for source in sources}), len(sources))
        for source in sources:
            self.assertTrue(
                source["key"].startswith("biomes/verdant_wilds/tiles/v3/"),
                source["key"],
            )
            self.assertNotIn("..", source["key"].split("/"))
            self.assertGreater(source["size"], 0)
            self.assertRegex(source["sha1"], SHA1)
            self.assertRegex(source["sha256"], SHA256)

    def test_mesh_contracts_have_source_and_ue_bounds_pivots_footprints_and_slots(self):
        self.assertIn("catalog_sha256", self.data["source"])
        if "catalog_sha256" not in self.data["source"]:
            return
        self.assertRegex(self.data["source"]["catalog_sha256"], SHA256)
        for mesh in self.meshes:
            with self.subTest(mesh=mesh["id"]):
                self.assertIn("source_bounds_cm", mesh)
                if "source_bounds_cm" not in mesh:
                    continue
                self.assertEqual(mesh["units"]["system"], "cm")
                self.assertEqual(mesh["units"]["grid_cm"], 200)
                self.assertEqual(mesh["units"]["fbx_unit_scale"], 0.01)
                self.assertTrue(mesh["pivot"])
                source_bounds = mesh["source_bounds_cm"]
                ue_bounds = mesh["bounds_cm"]
                self.assertEqual(len(source_bounds["min"]), 3)
                self.assertEqual(len(source_bounds["max"]), 3)
                self.assertEqual(len(ue_bounds["min"]), 3)
                self.assertEqual(len(ue_bounds["max"]), 3)
                self.assertEqual(
                    ue_bounds["min"],
                    [
                        source_bounds["min"][0],
                        -source_bounds["max"][1],
                        source_bounds["min"][2],
                    ],
                )
                self.assertEqual(
                    ue_bounds["max"],
                    [
                        source_bounds["max"][0],
                        -source_bounds["min"][1],
                        source_bounds["max"][2],
                    ],
                )
                self.assertTrue(all(
                    lo < hi for lo, hi in
                    zip(ue_bounds["min"], ue_bounds["max"])
                ))
                footprint = mesh["footprint"]
                self.assertTrue(footprint["rects_cm"])
                self.assertTrue(footprint["tile_rects"])
                for tile in footprint["tile_rects"]:
                    self.assertTrue(all(
                        isinstance(value, int) and value > 0
                        for value in tile["extent_tiles"]
                    ))
                    self.assertEqual(len(tile["pivot_offset_tiles"]), 2)
                self.assertEqual(len(footprint["z_cm"]), 2)
                self.assertTrue(mesh["material_slots"])
                self.assertEqual(
                    len(mesh["material_slots"]), len(set(mesh["material_slots"]))
                )
                self.assertTrue(mesh["texture_references"])
                expected_textures = {
                    texture
                    for slot in mesh["material_slots"]
                    for texture in self.data["material_textures"][slot].values()
                }
                self.assertEqual(set(mesh["texture_references"]), expected_textures)

    def test_hull_counts_are_copied_from_the_catalog_not_normalized(self):
        actual = {mesh["id"]: mesh["collision"]["count"] for mesh in self.meshes}
        self.assertEqual(actual, CATALOG_HULL_COUNTS)
        for mesh in self.meshes:
            self.assertEqual(
                len(mesh["collision"]["meshes"]), mesh["collision"]["count"], mesh["id"]
            )
            self.assertEqual(mesh["collision"]["type"], "UCX convex hulls")

    def test_the_coordinator_footprint_clarification_is_explicit(self):
        by_id = {mesh["id"]: mesh for mesh in self.meshes}
        straight = by_id["SM_VW_FeltCliff_Straight_01"]["footprint"]
        self.assertEqual(straight["rects_cm"], [[-100.0, 100.0, -200.0, 0.0]])
        self.assertEqual(straight["face_or_downhill_axes"], ["+Y"])
        self.assertEqual(straight["high_or_land_axes"], ["-Y"])

        inner = by_id["SM_VW_FeltCliff_InnerCorner_01"]["footprint"]
        self.assertEqual(
            inner["rects_cm"],
            [[-100.0, 100.0, -200.0, 0.0], [0.0, 200.0, -100.0, 100.0]],
        )
        ramp = by_id["SM_VW_FeltCliff_Ramp_2T_01"]
        self.assertEqual(ramp["catalog_properties"]["run_tiles"], 2)
        self.assertEqual(ramp["catalog_properties"]["width_cm"], 400.0)
        self.assertEqual(ramp["footprint"]["rects_cm"], [[-200.0, 200.0, -100.0, 400.0]])
        self.assertEqual(
            self.data["import_contract"]["camera_space"],
            {"yaw_deg": 0, "screen_north_axis": "+X", "screen_east_axis": "+Y"},
        )

    def test_revised_pond_contract_is_in_ue_space_and_defines_land_regions(self):
        contract = self.data["pond_collision_contracts"]
        self.assertEqual(contract["edge_xy_cm"], [-100.0, 100.0, -200.0, 200.0])
        self.assertEqual(contract["edge_x_tolerance_cm"], 15.0)
        self.assertEqual(contract["corner_xy_cm"], [-200.0, 200.0, -200.0, 200.0])
        self.assertEqual(contract["min_z_cm"], -200.0)
        self.assertEqual(contract["shallow_max_z_cm"], 15.0)
        self.assertEqual(contract["deep_wall_max_z_cm"], 250.0)
        self.assertEqual(contract["land_intrusion_limit_cm"], 15.0)
        pond_meshes = {
            mesh["id"]: mesh
            for mesh in self.meshes
            if mesh["kind"].startswith("pond_bank")
        }
        self.assertNotIn("actual_hull_extents_cm", contract)
        for mesh_id, mesh in pond_meshes.items():
            with self.subTest(mesh=mesh_id):
                self.assertIn("corner_land_region", mesh["collision"])
                if "corner_land_region" not in mesh["collision"]:
                    continue
                region = mesh["collision"]["corner_land_region"]
                if "_Edge_" in mesh_id:
                    self.assertEqual(region, {"operator": "half_plane", "y_lt_cm": -15.0})
                elif "_OuterCorner_" in mesh_id:
                    self.assertEqual(
                        region,
                        {"operator": "and", "x_lt_cm": -15.0, "y_lt_cm": -15.0},
                    )
                else:
                    self.assertEqual(
                        region,
                        {"operator": "or", "x_lt_cm": -15.0, "y_lt_cm": -15.0},
                    )

    def test_measured_values_are_not_baked_into_the_catalog_derived_manifest(self):
        for mesh in self.meshes:
            with self.subTest(mesh=mesh["id"]):
                footprint = mesh["footprint"]
                self.assertEqual(footprint["visual_overhang_limit_cm"], 15.0)
                self.assertNotIn("visual_overhang_downhill_cm", footprint)
                self.assertNotIn("visual_overhang_art_defect", footprint)

    def test_only_material_instance_sampled_textures_are_imported(self):
        self.assertEqual(len(self.textures), 12)
        self.assertEqual(
            set(self.data["material_textures"]),
            {"MI_VW_Grass", "MI_VW_CliffFace", "MI_VW_GrassToPath", "MI_VW_YarnRoot"},
        )
        expected = {
            "D": (True, "Default"),
            "N": (False, "Normalmap"),
            "ORM": (False, "Masks"),
        }
        for texture in self.textures:
            with self.subTest(texture=texture["id"]):
                suffix = texture["id"].rsplit("_", 1)[1]
                self.assertIn(suffix, expected)
                self.assertEqual(
                    (texture["srgb"], texture["compression"]), expected[suffix]
                )
                self.assertEqual(
                    texture["lod_group"],
                    "WorldNormalMap" if suffix == "N" else "World",
                )
                self.assertNotRegex(
                    texture["id"],
                    r"(CliffFace[BC]|Moss|WoodSlat|Pond|StitchSeam|_H$)",
                )

    def test_catalog_slot_mismatches_are_explicit_and_limited_to_six_pond_meshes(self):
        self.assertIn("catalog_slot_mismatch", self.data)
        if "catalog_slot_mismatch" not in self.data:
            return
        mismatches = self.data["catalog_slot_mismatch"]
        self.assertEqual(len(mismatches), 6)
        self.assertEqual(
            {entry["id"] for entry in mismatches},
            {
                "SM_VW_Pond_Edge_01",
                "SM_VW_Pond_OuterCorner_01",
                "SM_VW_Pond_InnerCorner_01",
                "SM_VW_Pond_Edge_Deep_01",
                "SM_VW_Pond_OuterCorner_Deep_01",
                "SM_VW_Pond_InnerCorner_Deep_01",
            },
        )
        for entry in mismatches:
            self.assertEqual(
                entry["catalog_material_slots"],
                ["MI_VW_Grass", "MI_VW_CliffFace", "MI_VW_YarnRoot"],
            )
            self.assertEqual(
                entry["fbx_material_slots"],
                ["MI_VW_Grass", "MI_VW_YarnRoot"],
            )

    def test_fixture_table_is_the_single_source_for_every_exact_actor_transform(self):
        self.assertIn("test_fixture", self.data)
        if "test_fixture" not in self.data:
            return
        fixture = self.data["test_fixture"]
        gallery = fixture["gallery"]
        lanes = fixture["lanes"]
        shallow_banks = fixture["shallow_bank_ring"]
        self.assertEqual(len(gallery), 17)
        self.assertEqual(len(lanes), 8)
        self.assertEqual(
            next(item for item in lanes if item["label"] == "VW_Lane_Ramp3T")["location_cm"],
            [200.0, 600.0, 0.0],
        )
        by_mesh = {item["mesh"]: item for item in gallery}
        ramp2 = by_mesh["SM_VW_FeltCliff_Ramp_2T_01"]["location_cm"]
        ramp3 = by_mesh["SM_VW_FeltCliff_Ramp_3T_01"]["location_cm"]
        self.assertGreaterEqual(
            ((ramp2[0] - ramp3[0]) ** 2 + (ramp2[1] - ramp3[1]) ** 2) ** 0.5,
            400.0,
        )
        for item in (*gallery, *lanes):
            self.assertEqual(item["scale"], [1.0, 1.0, 1.0])
            self.assertEqual(len(item["location_cm"]), 3)
            self.assertIsInstance(item["yaw_deg"], (int, float))
        compass = {item["label"]: item["location_cm"] for item in shallow_banks}
        self.assertTrue(all(location[0] == 2200.0 for label, location in compass.items()
                            if "_Bank_S_" in label))
        self.assertTrue(all(location[0] == 2800.0 for label, location in compass.items()
                            if "_Bank_N_" in label))
        self.assertTrue(all(location[1] == -300.0 for label, location in compass.items()
                            if "_Bank_W_" in label))
        self.assertTrue(all(location[1] == 300.0 for label, location in compass.items()
                            if "_Bank_E_" in label))

    def test_imported_facing_and_yaw_offset_are_measured_contract_fields(self):
        self.assertIn("orientation_contract", self.data)
        if "orientation_contract" not in self.data:
            return
        facing = self.data["orientation_contract"]
        self.assertEqual(facing["coordinate_conversion"], "source Y negated to UE Y")
        self.assertIn("yaw_offset_deg", facing)
        self.assertIn("straight_cliff_face_axis", facing)
        self.assertIn("ramp_2t_downhill_axis", facing)

    def test_tracked_files_are_credential_free_and_contain_no_raw_sources(self):
        tracked = subprocess.run(
            ["git", "ls-files", "-z"],
            cwd=PROJECT_ROOT,
            check=True,
            capture_output=True,
        ).stdout.split(b"\0")
        forbidden = (
            re.compile(rb"Authorization[:=]"),
            re.compile(rb"X-Bz-[A-Za-z-]*[:=]"),
            re.compile(rb"3_\d{14}_[0-9a-f]{16,}"),
        )
        text_suffixes = {
            ".bat", ".cpp", ".h", ".ini", ".json", ".md", ".mdc", ".ps1",
            ".py", ".toml", ".txt", ".uplugin", ".uproject", ".yaml", ".yml",
        }
        for raw_path in filter(None, tracked):
            path = os.path.join(PROJECT_ROOT, os.fsdecode(raw_path))
            lower = path.lower()
            self.assertFalse(lower.endswith(".fbx"), path)
            self.assertFalse(
                os.path.basename(path).startswith("T_VW_")
                and lower.endswith((".png", ".tga", ".jpg", ".jpeg")),
                path,
            )
            if os.path.splitext(lower)[1] not in text_suffixes:
                continue
            with open(path, "rb") as stream:
                contents = stream.read()
            for pattern in forbidden:
                self.assertIsNone(pattern.search(contents), path)


if __name__ == "__main__":
    unittest.main()
