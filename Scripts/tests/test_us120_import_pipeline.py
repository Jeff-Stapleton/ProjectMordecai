"""Offline RED/GREEN contracts for US-120's catalog and source pipeline."""

import hashlib
import importlib.util
import json
import os
import pathlib
import tempfile
import unittest
from unittest import mock


SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
FIXTURE = SCRIPTS / "tests" / "fixtures" / "us120_catalog_minimal.json"
MANIFEST = SCRIPTS / "us120_vw_source_manifest.json"
BUILDER = SCRIPTS / "us120_build_vw_manifest.py"
STAGER = SCRIPTS / "us120_stage_vw_sources.py"
IMPORTER = SCRIPTS / "us120_import_vw_tiles.py"
MAP_CREATOR = SCRIPTS / "us120_create_vw_tile_test_map.py"
TRIBAL = SCRIPTS.parent / ".claude" / "skills" / "unreal-tribal-knowledge" / "SKILL.md"


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class TestUs120ManifestBuilder(unittest.TestCase):
    def test_fixture_catalog_regenerates_a_canonical_ue_space_manifest(self):
        self.assertTrue(BUILDER.is_file(), BUILDER)
        builder = load_module(BUILDER, "us120_build_vw_manifest")
        raw = FIXTURE.read_bytes()
        catalog = json.loads(raw)
        manifest = builder.build_manifest(
            catalog,
            raw,
            expected_ready_ids=("SM_VW_FeltCliff_Straight_01",),
        )

        self.assertEqual(
            manifest["source"]["catalog_sha256"], hashlib.sha256(raw).hexdigest()
        )
        self.assertEqual([mesh["id"] for mesh in manifest["meshes"]],
                         ["SM_VW_FeltCliff_Straight_01"])
        mesh = manifest["meshes"][0]
        self.assertEqual(
            mesh["source_bounds_cm"],
            {"min": [-100.0, -24.9, -4.0], "max": [100.0, 100.0, 205.6]},
        )
        self.assertEqual(
            mesh["bounds_cm"],
            {"min": [-100.0, -100.0, -4.0], "max": [100.0, 24.9, 205.6]},
        )
        self.assertEqual(
            mesh["texture_references"],
            ["T_VW_Grass_D", "T_VW_Grass_N", "T_VW_Grass_ORM"],
        )
        self.assertEqual(
            [texture["id"] for texture in manifest["textures"]],
            ["T_VW_Grass_D", "T_VW_Grass_N", "T_VW_Grass_ORM"],
        )
        self.assertNotIn("SM_VW_ReviewOnly", json.dumps(manifest))

        first = builder.canonical_json(manifest)
        second = builder.canonical_json(
            builder.build_manifest(
                json.loads(raw),
                raw,
                expected_ready_ids=("SM_VW_FeltCliff_Straight_01",),
            )
        )
        self.assertEqual(first, second)
        self.assertTrue(first.endswith("\n"))

    def test_private_staged_catalog_exactly_regenerates_the_committed_manifest_when_present(self):
        catalog = pathlib.Path(os.environ.get("TEMP", tempfile.gettempdir()))
        catalog /= "ProjectMordecai-US120"
        catalog /= "catalog.json"
        if not catalog.is_file():
            self.skipTest("private staged catalog is not present on this worker")
        self.assertTrue(BUILDER.is_file(), BUILDER)
        builder = load_module(BUILDER, "us120_build_vw_manifest_full")
        raw = catalog.read_bytes()
        generated = builder.build_manifest(json.loads(raw), raw)
        self.assertEqual(
            builder.canonical_json(generated),
            MANIFEST.read_text(encoding="utf-8"),
        )


class TestUs120SecureStaging(unittest.TestCase):
    def test_stager_reads_authorization_only_from_environment(self):
        self.assertTrue(STAGER.is_file(), STAGER)
        stager = load_module(STAGER, "us120_stage_vw_sources")
        with mock.patch.dict(
            os.environ,
            {"US120_B2_AUTH_TOKEN": "fixture-secret-token"},
            clear=True,
        ):
            self.assertEqual(
                stager.download_headers(),
                {"Authorization": "fixture-secret-token"},
            )
        self.assertNotIn("fixture-secret-token", STAGER.read_text(encoding="utf-8"))

    def test_stager_hash_verifies_every_download(self):
        self.assertTrue(STAGER.is_file(), STAGER)
        stager = load_module(STAGER, "us120_stage_vw_sources_hash")
        with tempfile.TemporaryDirectory() as temp_dir:
            path = pathlib.Path(temp_dir) / "source.bin"
            path.write_bytes(b"verified fixture bytes")
            size, sha1, sha256 = stager.digest(path)
            entry = {"size": size, "sha1": sha1, "sha256": sha256}
            stager.verify_file(path, entry)
            path.write_bytes(b"tampered fixture bytes")
            with self.assertRaisesRegex(RuntimeError, "hash mismatch"):
                stager.verify_file(path, entry)


class TestUs120ImporterSourceContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = IMPORTER.read_text(encoding="utf-8")

    def test_import_task_requires_the_exact_reported_object_path(self):
        self.assertIn("if expected not in paths:", self.source)
        self.assertNotIn(
            "expected not in paths and not EAL.does_asset_exist", self.source
        )

    def test_import_task_rejects_stale_source_md5(self):
        self.assertIn("source_file_hash", self.source)
        self.assertIn("staged_source_md5", self.source)
        self.assertIn("source MD5 readback mismatch", self.source)

    def test_legacy_fbx_pipeline_and_settings_are_explicitly_read_back(self):
        for text in (
            "unreal.FbxFactory()",
            '"generate_lightmap_u_vs", False',
            '"build_nanite", True',
            "ImportedMaterialSlotName",
            "MaterialSlotName",
            "catalog_slot_mismatch",
        ):
            self.assertIn(text, self.source)
        self.assertNotIn("materials.append(unreal.StaticMaterial())", self.source)

    def test_missing_instance_texture_is_fatal_and_height_is_not_sampled(self):
        self.assertIn("missing texture for material parameter", self.source)
        self.assertNotIn('"HeightTexture"', self.source)

    def test_map_builder_never_reflects_imported_mesh_or_collision(self):
        source = MAP_CREATOR.read_text(encoding="utf-8")
        self.assertNotIn("GeometryScript_MeshTransforms.transform_mesh", source)
        self.assertNotIn("transform_simple_collision_shapes", source)
        self.assertNotIn("Vector(1.0, -1.0, 1.0)", source)

    def test_tribal_lesson_records_legacy_fbx_y_conversion_without_post_import_mirroring(self):
        source = TRIBAL.read_text(encoding="utf-8")
        self.assertIn("negates source Y", source)
        self.assertIn("do not mirror", source.lower())


if __name__ == "__main__":
    unittest.main()
