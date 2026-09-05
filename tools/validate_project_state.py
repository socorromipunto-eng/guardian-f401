#!/usr/bin/env python3
import argparse,hashlib,json,re,sys
from pathlib import Path
H40=re.compile(r"^[0-9a-fA-F]{40}$"); H64=re.compile(r"^[0-9a-fA-F]{64}$")
EXPECTED={"governance/requirements-register.json","governance/capability-register.json","governance/claims-register.json","governance/evidence-register.json","governance/document-register.json"}
def dg(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def bad(x): print("PROJECT_STATE_VALIDATION=FAIL"); print("REASON="+x); return 2
def main():
 a=argparse.ArgumentParser(); a.add_argument("--repo",default="."); n=a.parse_args(); r=Path(n.repo).resolve()
 try: d=json.loads((r/"governance/project-state.json").read_text(encoding="utf-8"))
 except Exception as e: return bad("PARSE:"+str(e))
 for k in ("schema_version","id","source_commit","source_tree","registers"):
  if k not in d: return bad("MISSING_FIELD:"+k)
 if d["id"]!="guardian:project-state:current": return bad("INVALID_ID")
 if not H40.fullmatch(str(d["source_commit"])): return bad("INVALID_SOURCE_COMMIT_FORMAT")
 if not H40.fullmatch(str(d["source_tree"])): return bad("INVALID_SOURCE_TREE_FORMAT")
 if not isinstance(d["registers"],list): return bad("REGISTERS_NOT_LIST")
 seen=set()
 for x in d["registers"]:
  for k in ("path","schema_id","schema_version","sha256"):
   if k not in x: return bad("REGISTER_MISSING_FIELD:"+k)
  q=x["path"]
  if q in seen: return bad("DUPLICATE_REGISTER:"+q)
  seen.add(q); f=r/q
  if not H64.fullmatch(str(x["sha256"])): return bad("INVALID_SHA256:"+q)
  if not f.is_file(): return bad("REGISTER_MISSING:"+q)
  actual=dg(f)
  if actual!=str(x["sha256"]).upper(): return bad("REGISTER_HASH_MISMATCH:"+q+":EXPECTED="+str(x["sha256"]).upper()+":ACTUAL="+actual)
 if seen!=EXPECTED: return bad("REGISTER_SET_MISMATCH")
 print("PROJECT_STATE_VALIDATION=PASS")
 print("REGISTER_HASH_COHERENCE=PASS")
 import subprocess
 obj = subprocess.run(
  ["git","-C",str(r),"cat-file","-t",str(d["source_commit"])],
  text=True,capture_output=True,shell=False
 )
 if obj.returncode!=0 or obj.stdout.strip()!="commit":
  return bad("SOURCE_COMMIT_NOT_COMMIT:"+str(d["source_commit"]))
 tree = subprocess.run(
  ["git","-C",str(r),"rev-parse",str(d["source_commit"])+"^{tree}"],
  text=True,capture_output=True,shell=False
 )
 if tree.returncode!=0:
  return bad("SOURCE_COMMIT_TREE_RESOLUTION_FAILED")
 actual_tree=tree.stdout.strip()
 if actual_tree!=str(d["source_tree"]):
  return bad("SOURCE_TREE_MISMATCH:EXPECTED="+str(d["source_tree"])+":ACTUAL="+actual_tree)
 print("SOURCE_COMMIT_SEMANTIC_BINDING=PASS")
 print("SOURCE_TREE_SEMANTIC_BINDING=PASS")
 print("SOURCE_COMMIT_EQUALS_HEAD=NOT_REQUIRED")
 print("DIRTY_WORKTREE_AS_SOURCE_TREE=PROHIBITED")
 return 0
if __name__=="__main__": sys.exit(main())
