#!/usr/bin/env python3
"""M17 portable host validation; no target or legal qualification."""
import argparse, hashlib, json, os, re, shutil, subprocess
from pathlib import Path

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()

def checked_path(root, rel):
    if not isinstance(rel, str) or not rel or "\\" in rel or ":" in rel or any(p in ("", ".", "..") for p in rel.split("/")):
        raise ValueError("UNSAFE_RECIPE_PATH")
    path = (root / rel).resolve()
    if not path.is_relative_to(root.resolve()):
        raise ValueError("RECIPE_PATH_ESCAPE")
    return path

def validate_manifest(root, manifest):
    if manifest.get("schema_version") != 1 or manifest.get("scope") != "portable_host_only":
        raise ValueError("MANIFEST_SCOPE_INVALID")
    recipes = manifest["recipes"]
    expected = {'01-multinode', '02-nxp-supervisor', '03-st-measurement', '04-witness', '05-high-compute', '06-signed-evidence', '10-monocypher', '11-anti-replay', '12-corroboration', '13-corroboration-wire', '14-ack', '15-signed-envelope'}
    if len(recipes) != 12 or {r["id"] for r in recipes} != expected:
        raise ValueError("RECIPE_SET_INVALID")
    used = {p for r in recipes for p in r["sources"]}
    if used != set(manifest["source_sha256"]):
        raise ValueError("SOURCE_MEMBERSHIP_INVALID")
    for rel, sha in manifest["source_sha256"].items():
        if digest(checked_path(root, rel)) != sha:
            raise ValueError("SOURCE_HASH_MISMATCH=" + rel)
    expected_headers = {p.relative_to(root).as_posix() for inc in {i for r in recipes for i in r["includes"]} for p in checked_path(root, inc).glob("*.h")}
    if set(manifest.get("header_sha256", {})) != expected_headers:
        raise ValueError("HEADER_MEMBERSHIP_INVALID")
    for rel, expected_sha in manifest["header_sha256"].items():
        if digest(checked_path(root, rel)) != expected_sha:
            raise ValueError("HEADER_HASH_MISMATCH=" + rel)
    for recipe in recipes:
        if len(recipe["sources"]) != len(set(recipe["sources"])):
            raise ValueError("DUPLICATE_TRANSLATION_UNIT")
        for inc in recipe["includes"]:
            if not checked_path(root, inc).is_dir():
                raise ValueError("INCLUDE_DIRECTORY_MISSING=" + inc)
    crypto = next(r for r in recipes if r["id"] == "10-monocypher")
    if "firmware/Firmware/Src/guardian_firmware_trusted_key_provider.c" in crypto["sources"]:
        raise ValueError("MONOCYPHER_TEST_OWNS_RESOLVERS")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--recipe", required=True)
    parser.add_argument("--optimization", choices=("O0", "O2"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    manifest_path = root / "tools/m17-host-test-recipes.json"
    compiler_name = shutil.which("gcc")
    if not compiler_name:
        raise SystemExit("GCC_NOT_FOUND_NO_INSTALL")
    compiler = Path(compiler_name).resolve()
    out = args.output.resolve()
    if out.exists():
        raise SystemExit("OUTPUT_ALREADY_EXISTS_NO_REUSE")
    out.mkdir(parents=True)
    report = {"result": "CONTROLLED_STOP", "scope": "portable_host_only", "recipe": args.recipe,
              "optimization": args.optimization, "commands": [], "objects": [],
              "physical_qualification": False, "legal_conformance": False, "production_key_qualification": False}
    def save():
        (out / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    def run(label, argv):
        row = {"label": label, "argv": [str(x) for x in argv]}
        try:
            process = subprocess.run(row["argv"], cwd=root, stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False, timeout=60)
            stdout, stderr = process.stdout, process.stderr
            row.update(exit_code=process.returncode, timed_out=False)
        except subprocess.TimeoutExpired as error:
            stdout, stderr = error.stdout or b"", error.stderr or b""
            row.update(exit_code=None, timed_out=True)
        (out / (label + "-stdout.txt")).write_bytes(stdout)
        (out / (label + "-stderr.txt")).write_bytes(stderr)
        row.update(stdout_sha256=hashlib.sha256(stdout).hexdigest().upper(),
                   stderr_sha256=hashlib.sha256(stderr).hexdigest().upper())
        report["commands"].append(row)
        save()
        if row["timed_out"] or row["exit_code"] != 0:
            raise ValueError("HOST_COMMAND_FAILED=" + label)
        return stdout.decode("utf-8", errors="replace")
    try:
        manifest_bytes = manifest_path.read_bytes()
        manifest = json.loads(manifest_bytes)
        validate_manifest(root, manifest)
        recipe = next(r for r in manifest["recipes"] if r["id"] == args.recipe)
        report.update(manifest_sha256=digest(manifest_path), compiler_path=str(compiler),
                      compiler_sha256=digest(compiler), source_sha256=manifest["source_sha256"])
        report["compiler_version"] = run("compiler-version", [compiler, "--version"])
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wsign-conversion",
                 "-Wshadow", "-Wcast-qual", "-Wvla", "-Werror", "-UNDEBUG", "-" + args.optimization]
        report["flags"] = flags
        includes = ["-I" + str(checked_path(root, p)) for p in recipe["includes"]]
        objects = []
        for index, rel in enumerate(recipe["sources"]):
            obj = out / (str(index) + ".o")
            run("compile-" + str(index), [compiler, *flags, *includes, "-c", checked_path(root, rel), "-o", obj])
            objects.append(obj)
            report["objects"].append({"source": rel, "artifact": obj.name, "sha256": digest(obj)})
        image = out / "host-test"
        run("link", [compiler, *objects, "-o", image])
        image_hash = digest(image)
        stdout = run("execute", [image])
        if stdout.splitlines().count(recipe["success_line"]) != 1:
            raise ValueError("EXACT_SUCCESS_MARKER_MISSING_OR_DUPLICATED")
        for required_line in recipe.get("required_lines", []):
            if stdout.splitlines().count(required_line) != 1:
                raise ValueError("REQUIRED_EVIDENCE_LINE_MISSING_OR_DUPLICATED=" + required_line)
        if digest(image) != image_hash:
            raise ValueError("EXECUTABLE_CHANGED")
        validate_manifest(root, manifest)
        if digest(compiler) != report["compiler_sha256"] or digest(manifest_path) != report["manifest_sha256"]:
            raise ValueError("TOOL_OR_MANIFEST_CHANGED")
        report.update(result="PASS_HOST_SCOPE_ONLY", executable_sha256=image_hash)
        print("M17_HOST_RESULT=PASS_HOST_SCOPE_ONLY")
        return 0
    except (ValueError, KeyError, OSError, StopIteration) as error:
        report["stop_reason"] = str(error)
        print("M17_HOST_RESULT=CONTROLLED_STOP: " + str(error))
        return 1
    finally:
        save()

if __name__ == "__main__":
    raise SystemExit(main())
