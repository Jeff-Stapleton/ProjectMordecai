#!/usr/bin/env python3
"""Hash-verified B2 staging for US-120.

Credentials are read only from environment variables and are never printed:
  US120_B2_AUTH_TOKEN
or:
  US120_B2_KEY_ID + US120_B2_APPLICATION_KEY
"""

import argparse
import base64
import hashlib
import json
import os
import pathlib
import tempfile
import urllib.parse
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_STAGE = pathlib.Path(
    os.environ.get("TEMP", tempfile.gettempdir())
) / "ProjectMordecai-US120"
DEFAULT_CATALOG = DEFAULT_STAGE / "catalog.json"
DEFAULT_DESTINATION = DEFAULT_STAGE / "sources"
DEFAULT_DOWNLOAD_ROOT = "https://f004.backblazeb2.com/file/mordecai-assets/"


def digest(path):
    sha1 = hashlib.sha1()
    sha256 = hashlib.sha256()
    size = 0
    with pathlib.Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            size += len(chunk)
            sha1.update(chunk)
            sha256.update(chunk)
    return size, sha1.hexdigest(), sha256.hexdigest()


def verify_file(path, source):
    actual = digest(path)
    expected = (source["size"], source["sha1"], source["sha256"])
    if actual != expected:
        raise RuntimeError(f"hash mismatch for staged file {path}")


def _authorize_with_key(env):
    key_id = env.get("US120_B2_KEY_ID")
    application_key = env.get("US120_B2_APPLICATION_KEY")
    if not key_id or not application_key:
        raise RuntimeError(
            "set US120_B2_AUTH_TOKEN or both US120_B2_KEY_ID and "
            "US120_B2_APPLICATION_KEY"
        )
    basic = base64.b64encode(
        f"{key_id}:{application_key}".encode("utf-8")
    ).decode("ascii")
    request = urllib.request.Request(
        "https://api.backblazeb2.com/b2api/v3/b2_authorize_account",
        headers={"Authorization": f"Basic {basic}"},
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        payload = json.load(response)
    token = payload.get("authorizationToken")
    if not token:
        raise RuntimeError("B2 key authorization returned no token")
    return token


def download_headers(env=None):
    env = os.environ if env is None else env
    token = env.get("US120_B2_AUTH_TOKEN") or _authorize_with_key(env)
    return {"Authorization": token}


def required_sources(manifest):
    entries = [mesh["source"] for mesh in manifest["meshes"]]
    entries.extend(texture["source"] for texture in manifest["textures"])
    return entries


def safe_destination(root, key):
    parts = pathlib.PurePosixPath(key).parts
    if not parts or any(part in ("", ".", "..") for part in parts):
        raise RuntimeError(f"unsafe catalog key {key!r}")
    destination = pathlib.Path(root).joinpath(*parts).resolve()
    root = pathlib.Path(root).resolve()
    if root not in destination.parents:
        raise RuntimeError(f"catalog key escapes staging root: {key!r}")
    return destination


def download(source, destination, headers, root_url=DEFAULT_DOWNLOAD_ROOT):
    destination = pathlib.Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    part = destination.with_name(destination.name + ".part")
    url = urllib.parse.urljoin(root_url, urllib.parse.quote(source["key"], safe="/"))
    request = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=180) as response, part.open("wb") as stream:
            while True:
                chunk = response.read(1024 * 1024)
                if not chunk:
                    break
                stream.write(chunk)
        verify_file(part, source)
        part.replace(destination)
    finally:
        if part.exists():
            part.unlink()


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", type=pathlib.Path, default=DEFAULT_CATALOG)
    parser.add_argument(
        "--manifest",
        type=pathlib.Path,
        default=ROOT / "Scripts" / "us120_vw_source_manifest.json",
    )
    parser.add_argument(
        "--destination", type=pathlib.Path, default=DEFAULT_DESTINATION
    )
    args = parser.parse_args(argv)

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    catalog_sha256 = hashlib.sha256(args.catalog.read_bytes()).hexdigest()
    if catalog_sha256 != manifest["source"]["catalog_sha256"]:
        raise RuntimeError("staged catalog hash differs from committed manifest")
    headers = None
    sources = required_sources(manifest)
    for index, source in enumerate(sources, 1):
        destination = safe_destination(args.destination, source["key"])
        if destination.is_file():
            verify_file(destination, source)
            status = "verified"
        else:
            if headers is None:
                headers = download_headers()
            download(source, destination, headers)
            status = "downloaded"
        print(f"[{index}/{len(sources)}] {status}: {pathlib.PurePosixPath(source['key']).name}")
    print(f"US-120 staging complete: {len(sources)} hash-verified files")


if __name__ == "__main__":
    main()
