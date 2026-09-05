#!/usr/bin/env python3
from __future__ import annotations
import argparse, subprocess
from pathlib import Path
TEXT_SUFFIXES={".c",".h",".py",".md",".yml",".yaml",".json",".txt",".cff",".csv",".toml",".ini",".cfg",".cmake",".ps1",".sh"}
TEXT_NAMES={"VERSION","CMakeLists.txt","Makefile"}
def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--repo",default="."); a=ap.parse_args(); repo=Path(a.repo).resolve()
    r=subprocess.run(["git","-C",str(repo),"ls-files","-z"],capture_output=True,text=True,shell=False)
    if r.returncode:
        print("GUARDIAN_REPOSITORY_ENCODING_V1\nFINAL=FAIL\nERROR="+r.stderr.strip()); return 2
    bom=[]; bad=[]; count=0
    for rel in sorted(x for x in r.stdout.split("\0") if x):
        p=Path(rel)
        if p.name not in TEXT_NAMES and p.suffix.lower() not in TEXT_SUFFIXES: continue
        data=(repo/rel).read_bytes(); count+=1
        if data.startswith(b"\xef\xbb\xbf"): bom.append(rel)
        try: data.decode("utf-8",errors="strict")
        except UnicodeDecodeError: bad.append(rel)
    print("GUARDIAN_REPOSITORY_ENCODING_V1"); print(f"TRACKED_TEXT_CHECKED={count}")
    print(f"UTF8_BOM_COUNT={len(bom)}"); [print("UTF8_BOM="+x) for x in bom]
    print(f"INVALID_UTF8_COUNT={len(bad)}"); [print("INVALID_UTF8="+x) for x in bad]
    ok=not bom and not bad; print("FINAL="+("PASS" if ok else "FAIL")); return 0 if ok else 1
if __name__=="__main__": raise SystemExit(main())
