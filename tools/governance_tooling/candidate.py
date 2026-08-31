"""Candidate integrity and evidence-binding gate."""
from __future__ import annotations
from dataclasses import dataclass
import hashlib
from .evidence import DEFAULT_NOT_PROVEN
from .repository import GitRepository
from .result import Result

FORBIDDEN_PROVEN_CLAIMS=frozenset(DEFAULT_NOT_PROVEN)

@dataclass(frozen=True,slots=True)
class ArtifactIdentity:
    path:str
    worktree_sha256:str
    worktree_bytes:int
    index_sha256:str|None=None
    index_bytes:int|None=None
    @property
    def index_matches_worktree(self)->bool|None:
        if self.index_sha256 is None or self.index_bytes is None:return None
        return self.worktree_sha256==self.index_sha256 and self.worktree_bytes==self.index_bytes

def _sha(data:bytes)->str:return hashlib.sha256(data).hexdigest().upper()

def evidence_boundary_is_coherent(result:Result)->bool:
    p=set(result.proven); n=set(result.not_proven)
    return not (p & n) and not bool(p & FORBIDDEN_PROVEN_CLAIMS)

def _identity(repo:GitRepository,path:str,staged:set[str])->ArtifactIdentity:
    wt=(repo.root/path).read_bytes()
    if path not in staged:return ArtifactIdentity(path,_sha(wt),len(wt))
    idx=repo.index_blob(path)
    return ArtifactIdentity(path,_sha(wt),len(wt),_sha(idx),len(idx))

def run_candidate(repo:GitRepository,allow:tuple[str,...],forbid:tuple[str,...]=())->Result:
    r=Result(command='candidate',repository=str(repo.root))
    branch=repo.branch(); head=repo.head(); staged=set(repo.staged()); tracked=set(repo.tracked_changes()); untracked=set(repo.untracked())
    observed=staged|tracked|untracked; allowed=set(allow); forbidden=set(forbid)
    missing=allowed-observed; unexpected=observed-allowed; forbidden_present=observed&forbidden
    r.source_of_truth=head
    r.add('INSIDE_WORK_TREE',repo.is_inside_work_tree())
    r.add('FEATURE_BRANCH',bool(branch) and branch!='main',branch or 'DETACHED_HEAD')
    r.add('ALLOWLIST_NONEMPTY',bool(allowed),f'count={len(allowed)}')
    r.add('EXACT_SCOPE',not missing and not unexpected,f'missing={len(missing)},unexpected={len(unexpected)}')
    r.add('FORBIDDEN_SCOPE_ABSENT',not forbidden_present,f'count={len(forbidden_present)}')
    failures=[]; identities=[]
    for p in sorted(observed&allowed):
        if not (repo.root/p).is_file(): failures.append(p); continue
        ident=_identity(repo,p,staged); identities.append(ident)
        if ident.index_matches_worktree is False: failures.append(p)
    r.add('ARTIFACT_IDENTITY',not failures,f'checked={len(identities)},failures={len(failures)}')
    for i in identities:
        r.proven.append(f'artifact {i.path} worktree sha256={i.worktree_sha256} bytes={i.worktree_bytes}')
        if i.index_sha256 is not None:r.proven.append(f'artifact {i.path} index sha256={i.index_sha256} bytes={i.index_bytes}')
    r.proven += [f'current branch is {branch or "DETACHED"}',f'HEAD is {head}',f'candidate observed file count is {len(observed)}',f'candidate allowlist file count is {len(allowed)}',f'staged file count is {len(staged)}',f'tracked change count is {len(tracked)}',f'untracked file count is {len(untracked)}']
    r.not_proven.extend(DEFAULT_NOT_PROVEN)
    r.not_proven += ['semantic correctness of candidate contents','human approval of candidate scope','human authorization to commit, push, create a pull request, or merge']
    r.add('EVIDENCE_BOUNDARY',evidence_boundary_is_coherent(r),'PROVEN and NOT_PROVEN disjoint; forbidden claims not PROVEN')
    r.add('AUTHORITY_BOUNDARY',all(v=='NOT_GRANTED' for v in r.authority.values()),'COMMIT/PUSH/PR/MERGE remain NOT_GRANTED')
    return r
