"""Authority policy for read-only tooling v0.1."""

READ_ONLY_V1 = True
MUTATING_ACTIONS = frozenset(
    {"branch", "stage", "commit", "push", "pr", "merge", "delete", "tag", "release"}
)


def mutation_allowed(action: str) -> bool:
    del action
    return False
