#!/usr/bin/env python3
import argparse,hashlib,json,re,sys
from pathlib import Path
H64=re.compile(r"^[0-9a-fA-F]{64}$")
def dg(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def bad(x): print("DOCUMENT_REGISTER_VALIDATION=FAIL"); print("REASON="+x); return 2
def main():
 a=argparse.ArgumentParser(); a.add_argument("--repo",default="."); n=a.parse_args(); r=Path(n.repo).resolve()
 try: d=json.loads((r/"governance/document-register.json").read_text(encoding="utf-8"))
 except Exception as e: return bad("PARSE:"+str(e))
 if "schema_version" not in d or "documents" not in d: return bad("MISSING_REQUIRED_TOP_LEVEL_FIELD")
 if not isinstance(d["documents"],list): return bad("DOCUMENTS_NOT_LIST")
 paths=set(); ids=set()
 for x in d["documents"]:
  if not isinstance(x,dict) or "sha256" not in x or "path" not in x: return bad("MALFORMED_DOCUMENT_ENTRY")
  q=str(x["path"]); i=x.get("id")
  if q in paths: return bad("DUPLICATE_DOCUMENT_PATH:"+q)
  paths.add(q)
  if i is not None:
   if i in ids: return bad("DUPLICATE_DOCUMENT_ID:"+str(i))
   ids.add(i)
  f=r/q
  if not H64.fullmatch(str(x["sha256"])): return bad("INVALID_SHA256:"+q)
  if not f.is_file(): return bad("DOCUMENT_MISSING:"+q)
  actual=dg(f)
  if actual!=str(x["sha256"]).upper(): return bad("DOCUMENT_HASH_MISMATCH:"+q)
 print("DOCUMENT_REGISTER_VALIDATION=PASS")
 print("REGISTERED_DOCUMENT_INTEGRITY=PASS")
 print("REGISTERED_DOES_NOT_IMPLY_APPROVED=PASS")
 print("REGISTERED_DOES_NOT_IMPLY_IMPLEMENTED=PASS")
 print("REGISTERED_DOES_NOT_IMPLY_VALIDATED=PASS")
 print("REGISTERED_DOES_NOT_IMPLY_EVIDENCE_PRESENT=PASS")
 print("CONTROLLED_DOCUMENT_COMPLETENESS=NOT_DEMONSTRATED")
 return 0
if __name__=="__main__": sys.exit(main())
