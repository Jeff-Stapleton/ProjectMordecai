"""Offline contract tests for the ready-only US-120 source manifest.

The private B2 catalog remains authoritative.  The committed manifest is its deterministic,
credential-free import allowlist; raw FBX/PNG files are never part of the repository.
"""

import json
import os
import re
import unittest


SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MANIFEST = os.path.join(SCRIPTS, "us120_vw_source_manifest.json")

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

    def test_every_source_key_and_checksum_is_complete_and_unique(self):
        sources = [entry["source"] for entry in (*self.meshes, *self.textures)]
        self.assertEqual(len(sources), 51)
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

    def test_mesh_contracts_have_catalog_bounds_pivots_footprints_and_slots(self):
        for mesh in self.meshes:
            with self.subTest(mesh=mesh["id"]):
                self.assertEqual(mesh["units"]["system"], "cm")
                self.assertEqual(mesh["units"]["grid_cm"], 200)
                self.assertEqual(mesh["units"]["fbx_unit_scale"], 0.01)
                self.assertTrue(mesh["pivot"])
                self.assertEqual(len(mesh["bounds_cm"]["min"]), 3)
                self.assertEqual(len(mesh["bounds_cm"]["max"]), 3)
                self.assertTrue(all(
                    lo < hi for lo, hi in
                    zip(mesh["bounds_cm"]["min"], mesh["bounds_cm"]["max"])
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
        self.assertEqual(straight["rects_cm"], [[-100.0, 100.0, 0.0, 200.0]])
        self.assertEqual(straight["face_or_downhill_axes"], ["-Y"])
        self.assertEqual(straight["high_or_land_axes"], ["+Y"])

        inner = by_id["SM_VW_FeltCliff_InnerCorner_01"]["footprint"]
        self.assertEqual(
            inner["rects_cm"],
            [[-100.0, 100.0, 0.0, 200.0], [0.0, 200.0, -100.0, 100.0]],
        )
        ramp = by_id["SM_VW_FeltCliff_Ramp_2T_01"]
        self.assertEqual(ramp["catalog_properties"]["run_tiles"], 2)
        self.assertEqual(ramp["catalog_properties"]["width_cm"], 400.0)
        self.assertEqual(ramp["footprint"]["rects_cm"], [[-200.0, 200.0, -400.0, 100.0]])
        self.assertEqual(
            self.data["import_contract"]["camera_space"],
            {"yaw_deg": 0, "screen_north_axis": "+X", "screen_east_axis": "+Y"},
        )

    def test_revised_pond_contract_and_measured_hull_extents_are_complete(self):
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
        extents = contract["actual_hull_extents_cm"]
        self.assertEqual(set(extents), set(pond_meshes))
        for mesh_id, mesh in pond_meshes.items():
            with self.subTest(mesh=mesh_id):
                self.assertEqual(len(extents[mesh_id]), mesh["collision"]["count"])
                for hull in extents[mesh_id]:
                    self.assertEqual(len(hull["min"]), 3)
                    self.assertEqual(len(hull["max"]), 3)
                    self.assertTrue(
                        all(lo <= hi for lo, hi in zip(hull["min"], hull["max"]))
                    )

    def test_every_downhill_overhang_is_measured_and_flagged_without_editing_sources(self):
        for mesh in self.meshes:
            with self.subTest(mesh=mesh["id"]):
                footprint = mesh["footprint"]
                self.assertGreaterEqual(footprint["visual_overhang_downhill_cm"], 0.0)
                self.assertEqual(footprint["visual_overhang_limit_cm"], 15.0)
                self.assertEqual(
                    footprint["visual_overhang_art_defect"],
                    footprint["visual_overhang_downhill_cm"] > 15.0,
                )

    def test_ready_shared_textures_have_the_required_color_and_compression_contract(self):
        self.assertEqual(len(self.textures), 34)
        expected = {
            "D": (True, "Default"),
            "DA": (True, "Default"),
            "N": (False, "Normalmap"),
            "ORM": (False, "Masks"),
            "H": (False, "Grayscale"),
        }
        for texture in self.textures:
            with self.subTest(texture=texture["id"]):
                suffix = texture["id"].rsplit("_", 1)[1]
                self.assertIn(suffix, expected)
                self.assertEqual(
                    (texture["srgb"], texture["compression"]), expected[suffix]
                )

    def test_manifest_is_credential_free_and_raw_sources_are_not_beside_it(self):
        text = open(MANIFEST, encoding="utf-8").read()
        self.assertNotIn("Authorization=", text)
        self.assertNotRegex(text, r"3_\d{14}_[0-9a-f]+")
        for root, _, files in os.walk(SCRIPTS):
            for filename in files:
                lower = filename.lower()
                is_fbx = lower.endswith(".fbx")
                is_vw_texture = (
                    filename.startswith("T_VW_")
                    and lower.endswith((".png", ".tga", ".jpg", ".jpeg"))
                )
                self.assertFalse(is_fbx or is_vw_texture, os.path.join(root, filename))


if __name__ == "__main__":
    unittest.main()
