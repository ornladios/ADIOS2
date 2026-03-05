---
name: Backport PR
about: Cherry-pick a merged PR onto a release branch
labels: backport
---

<!--
  Fill in the three fields below once; the rest of the issue references them by name.
-->
- **PR**: #<!-- number -->
- **Title**: <!-- e.g. "Fix BP5 metadata corruption" -->
- **Target branch**: `<!-- e.g. release_211 -->`

---

Please backport the PR above to the target branch.

## Steps

1. Find the merge commit for the PR in the ornladios/ADIOS2 repository
2. Cherry-pick it onto the target branch
3. Push to a branch named `backport-to-<target>-<pr>` in ornladios-robot/ADIOS2
4. Open a pull request from ornladios-robot/ADIOS2 targeting the target branch
   in ornladios/ADIOS2 with title "Backport <PR> to <target>"

## Conflict resolution

If the cherry-pick produces merge conflicts, resolve them as follows:
- Prefer the incoming change (from the PR being backported) when it is a pure bug fix
  or a self-contained feature with no dependencies on master-only APIs.
- When the conflict is caused by a refactor or API change that only exists on master,
  adapt the backported code to use the equivalent API available on the target branch
  so the result compiles and the existing tests pass.
- Do NOT resolve conflicts by simply accepting the target-branch side and dropping the
  fix — the goal is to land the intended change on the release branch.

## ABI/API safety rules

When resolving conflicts or adapting code, strictly follow these rules to avoid
breaking the stable release branch:
- Do NOT change the signature, return type, or parameter order of any existing
  public C, C++, or Fortran API functions or classes.
- Do NOT add, remove, or reorder fields in any public struct or class that is part
  of the ABI-stable interface (e.g., anything in `bindings/`, `source/adios2/core/`,
  or installed public headers).
- Do NOT change the size or layout of any existing public type.
- New symbols may be added only if they are purely additive and do not alter the
  layout of existing types.
- If the backport cannot be cleanly adapted without breaking ABI/API, leave a comment
  in the pull request explaining why and request a human review before merging.
