# UG fork: new file, not in upstream OpenViBE. See UG.md, 2.50.
"""Install an official Qt build for Windows from Qt's online repository, without an account.

Used by the "Windows installer" workflow. aqtinstall 3.3, the tool the install-qt-action runs,
does not read the repository layout Qt uses from 6.11 on (one folder per compiler), so this
script does what the Maintenance Tool does for the packages we need:

  1. read Updates.xml of qt6_<ver>_<arch> on download.qt.io;
  2. for the Qt package and each add-on asked for, download every archive its Extract
     operations name (download.qt.io redirects to a mirror);
  3. check each archive against the SHA-256 that download.qt.io itself serves;
  4. unpack it where the operation says, and write bin/qt.conf as the Maintenance Tool does.

    python install-qt.py --version 6.12.0 --arch msvc2022_64 --dir C:\\Qt \\
        --modules qtcharts qtgraphs qtquick3d qtshadertools qt5compat

Qt ends up in <dir>/<version>/<arch>, the same folder the Maintenance Tool uses.
"""
import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET

REPO = "https://download.qt.io/online/qtsdkrepository/windows_x86/desktop"
TRUSTED_HOST = "download.qt.io"


def fetch(url, attempts=4):
    for attempt in range(1, attempts + 1):
        try:
            with urllib.request.urlopen(url, timeout=120) as response:
                return response.read(), response.geturl()
        except OSError as error:
            if attempt == attempts:
                raise
            print(f"  {error}; again in {5 * attempt} s", flush=True)
            time.sleep(5 * attempt)


def download(url, path, attempts=4):
    for attempt in range(1, attempts + 1):
        try:
            with urllib.request.urlopen(url, timeout=120) as response, open(path, "wb") as out:
                shutil.copyfileobj(response, out, 1 << 20)
            return
        except OSError as error:
            if attempt == attempts:
                raise
            print(f"  {error}; again in {5 * attempt} s", flush=True)
            time.sleep(5 * attempt)


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def find_7z():
    for candidate in (shutil.which("7z"), r"C:\Program Files\7-Zip\7z.exe"):
        if candidate and os.path.isfile(candidate):
            return candidate
    sys.exit("7-Zip (7z.exe) not found")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", required=True, help="e.g. 6.12.0")
    parser.add_argument("--arch", default="msvc2022_64")
    parser.add_argument("--dir", required=True, help="Qt root, e.g. C:\\Qt")
    parser.add_argument("--modules", nargs="*", default=[], help="add-ons, e.g. qtcharts qt5compat")
    args = parser.parse_args()

    v = args.version.replace(".", "")
    folder = f"{REPO}/qt6_{v}/qt6_{v}_{args.arch}"
    wanted = [f"qt.qt6.{v}.win64_{args.arch}"] + [f"qt.qt6.{v}.addons.{m}.win64_{args.arch}" for m in args.modules]

    print(f"Reading {folder}/Updates.xml", flush=True)
    updates = ET.fromstring(fetch(f"{folder}/Updates.xml")[0])
    packages = {p.findtext("Name"): p for p in updates.findall("PackageUpdate")}
    missing = [name for name in wanted if name not in packages]
    if missing:
        sys.exit(f"Not in Qt's repository: {', '.join(missing)}")

    seven_zip = find_7z()
    target_root = os.path.abspath(args.dir)
    with tempfile.TemporaryDirectory() as temp:
        for name in wanted:
            package = packages[name]
            version = package.findtext("Version")
            for operation in package.find("Operations"):
                if operation.get("name") != "Extract":
                    continue
                destination, archive = (a.text for a in operation)
                if not destination.startswith("@TargetDir@"):
                    sys.exit(f"{name}: unexpected destination {destination}")
                destination = os.path.join(target_root, os.path.normpath(destination[len("@TargetDir@/"):]))
                url = f"{folder}/{name}/{version}{archive}"

                text, checksum_url = fetch(url + ".sha256")
                if urllib.parse.urlparse(checksum_url).hostname != TRUSTED_HOST:
                    sys.exit(f"{archive}: checksum served by {checksum_url}, not by {TRUSTED_HOST}")
                expected = text.decode("ascii").split()[0].lower()

                print(f"{name}: {archive}", flush=True)
                local = os.path.join(temp, archive)
                download(url, local)
                actual = sha256_of(local)
                if actual != expected:
                    sys.exit(f"{archive}: SHA-256 {actual}, expected {expected}")
                os.makedirs(destination, exist_ok=True)
                subprocess.run([seven_zip, "x", "-y", "-bso0", "-bsp0", f"-o{destination}", local], check=True)
                os.remove(local)

    prefix = os.path.join(target_root, args.version, args.arch)
    with open(os.path.join(prefix, "bin", "qt.conf"), "w", encoding="ascii") as f:
        f.write("[Paths]\nPrefix=..\n")
    print(f"Qt {args.version} installed in {prefix}")


if __name__ == "__main__":
    main()
