"""Package an x86 plugin, verifying binary architecture and SDK exports."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
NAME = "Addon-Sscanf"
EXPORTS = {"PluginMain", "PluginUnload", "GetPluginApiVersion", "GetPluginSDKSignature"}


def pe_exports(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[:2] != b"MZ" or data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Not a PE binary")
    machine, count = struct.unpack_from("<HH", data, pe + 4)
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if machine != 0x14C or struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError("Expected an x86 DLL")
    if not struct.unpack_from("<H", data, pe + 22)[0] & 0x2000:
        raise ValueError("PE binary is not a DLL")
    sections = optional + optional_size

    def offset(rva):
        for i in range(count):
            section = sections + 40 * i
            virtual_size, address, raw_size, raw = struct.unpack_from("<IIII", data, section + 8)
            if address <= rva < address + max(virtual_size, raw_size):
                return raw + rva - address
        raise ValueError("Unmapped PE RVA")

    export_rva = struct.unpack_from("<I", data, optional + 96)[0]
    directory = offset(export_rva)
    names_count = struct.unpack_from("<I", data, directory + 24)[0]
    names = offset(struct.unpack_from("<I", data, directory + 32)[0])
    result = set()
    for i in range(names_count):
        start = offset(struct.unpack_from("<I", data, names + i * 4)[0])
        result.add(data[start:data.index(b"\0", start)].decode("ascii"))
    return result


def revision(path, allow_uncommitted):
    result = subprocess.run(["git", "rev-parse", "--verify", "HEAD"], cwd=path,
                            text=True, capture_output=True)
    if result.returncode:
        if allow_uncommitted:
            return "uncommitted"
        raise RuntimeError("Commit the reviewed source before creating release packages")
    return result.stdout.strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("platform", choices=["windows-x86", "linux-x86"])
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--allow-uncommitted", action="store_true", help="Local packaging validation only")
    args = parser.parse_args()
    windows = args.platform == "windows-x86"
    build = ROOT / args.build_dir
    binary = build / "Release" / (NAME + ".dll") if windows else build / (NAME + ".so")
    data = binary.read_bytes()
    if windows:
        symbols = pe_exports(data)
    else:
        if data[:6] != b"\x7fELF\x01\x01" or struct.unpack_from("<H", data, 18)[0] != 3:
            raise ValueError("Expected a little-endian x86 ELF library")
        output = subprocess.check_output(["nm", "-D", "--defined-only", str(binary)], text=True)
        symbols = {line.split()[-1] for line in output.splitlines() if line.split()}
    if not EXPORTS.issubset(symbols):
        raise ValueError("Missing plugin exports: " + ", ".join(sorted(EXPORTS - symbols)))
    status = subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True)
    if status and not args.allow_uncommitted:
        raise RuntimeError("Release packaging requires a clean source checkout")
    readiness = json.loads((ROOT / ".github/release.json").read_text(encoding="utf-8"))
    manifest = {"repository": revision(ROOT, args.allow_uncommitted),
                "sdk": revision(ROOT / "SDK", False), "platform": args.platform,
                "api_version": 1, "dirty": bool(status), "release_readiness": readiness,
                "binary_sha256": hashlib.sha256(data).hexdigest()}
    destination = ROOT / "dist"
    destination.mkdir(exist_ok=True)
    filename = NAME + "-" + args.platform + (".zip" if windows else ".tar.gz")
    archive_path = destination / filename
    with tempfile.TemporaryDirectory() as temporary:
        staging = Path(temporary) / NAME
        staging.mkdir()
        shutil.copy2(binary, staging / binary.name)
        for entry in ("README.md", "LICENSE", "docs", "examples", "THIRD_PARTY.md"):
            source = ROOT / entry
            if source.exists():
                target = staging / entry
                shutil.copytree(source, target) if source.is_dir() else shutil.copy2(source, target)
        (staging / "build-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        if windows:
            with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
                for source in sorted(staging.rglob("*")):
                    if source.is_file():
                        archive.write(source, source.relative_to(staging.parent))
        else:
            with tarfile.open(archive_path, "w:gz") as archive:
                archive.add(staging, arcname=NAME)
    digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
    (destination / (filename + ".sha256")).write_text(digest + "  " + filename + "\n", encoding="ascii")
    print("Verified x86 architecture and all four SDK exports: " + str(archive_path))


if __name__ == "__main__":
    main()
