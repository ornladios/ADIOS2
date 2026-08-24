***********
Backporting
***********

ADIOS2 uses the `korthout/backport-action <https://github.com/korthout/backport-action>`_
GitHub Action to automate backporting merged pull requests. Labeling a merged
PR with ``backport <branch>`` opens a cherry-pick PR against ``<branch>``,
for example:

- ``backport release_212`` — backports to the ``release_212`` branch.
- ``backport master`` — backports to ``master``. Use this for fixes merged
  directly into a release branch that also apply to ``master``.

Any target branch works, not just the currently active release line: apply
``backport release_28`` to send a fix to an older, still-maintained release
branch. Labels for ``master`` and the currently supported release branches
already exist; create a new ``backport release_X.Y`` label
(``gh label create "backport release_X.Y"``) the first time you need to
target a branch that doesn't have one yet.

The workflow is defined in ``.github/workflows/backport.yml``. It cherry-picks
the merged PR's commits with ``-x`` and opens a new PR against the target
branch for review; nothing is pushed directly.

A manual fallback, ``.github/workflows/backport-manual.yml``, is also
available via ``workflow_dispatch`` (PR number and target branch as inputs)
for cases where the label-driven workflow cannot be used. Like the
label-driven workflow, it only ever pushes to a new branch and opens a PR
for review; it never pushes or merges directly into the target branch.

When cutting a new release branch, create its ``backport release_X.Y`` label.
See the release checklist in ``.github/ISSUE_TEMPLATE/new_release.md``.

While a release line is active, its branch only receives forward
cherry-picks via ``backport release_X.Y`` labels; patch releases do not
merge anything back into ``master``. When a release line reaches its
final release, it is merged back into ``master`` with a normal
``git merge release_X.Y`` (see the release checklist).
