.. SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
..
.. SPDX-License-Identifier: Apache-2.0

<!--
  Replace the following vars with its corresponding values:
  - @VERSION@ (example 2.9.0)
  - @MAJOR@ (example 2)
  - @MINOR@ (example 9)
  - @OLD_RELEASE@ (example 2.8.3)

-->
Instructions for performing an ADIOS2 release:

- [ ] Make sure that the milestone for @VERSION@ has no pending issues/PRs.
- [ ] Update your remotes
``
git fetch origin
git fetch github #if needed
``
- [ ] Create a branch that updates the version
<!-- If the release_@MAJOR@@MINOR@ already exists -->
```
git checkout -b bump-release-version origin/release_@MAJOR@@MINOR@
```
<!-- else -->
```
git checkout -b bump-release-version origin/master
```
<!-- endif -->
- [ ] Add Commit that updates the version in the repo
```
git grep --name-only @OLD_RELEASE@ | xargs -n1 sed -i 's/@OLD_RELEASE@/@VERSION@/g'
git commit -am 'Bump version to v@VERSION@'
git push
```
- [ ] Create PR (BASE to master if release_@MAJOR@@MINOR@ does not exists; otherwise release_@MAJOR@@MINOR@)
- [ ] Ask for review
- [ ] Merge PR
- [ ] Create Tag commit `git tag -a -m 'v@VERSION' v@VERSION@ the_merge_commit`
- [ ] Create Release in GitHub page
  - Use the following script for getting the PR of this release
    - `./scripts/developer/create-changelog.sh v@VERSION@ v@OLD_RELEASE@`
  - Copy its content to the release description
<!-- If the release_@MAJOR@@MINOR@ does not exists -->
- [ ] Create the release_@MAJOR@@MINOR@ branch
```
git fetch origin
git checkout -b release_@MAJOR@@MINOR@ origin/master
# Use the following command with care
git push origin release_@MAJOR@@MINOR@:release_@MAJOR@@MINOR@
```
- [ ] Create the `backport release_@MAJOR@@MINOR@` label so PRs can be
  backported to the new branch (see
  [backporting docs](https://adios2.readthedocs.io/en/latest/advice/backporting.html)):
```
gh label create "backport release_@MAJOR@@MINOR@" --description "Backport to release_@MAJOR@@MINOR@" --color e99695
```
<!-- else -->
- [ ] Remove older patch releases for @MAJOR@.@MINOR@.X in ReadTheDocs.
<!-- endif -->
<!-- If this is the FINAL planned release for release_@MAJOR@@MINOR@ (not a patch release) -->
- [ ] Merge release_@MAJOR@@MINOR@ back into master, since this release line is complete:
```
git fetch origin
git checkout master
git reset --hard origin/master
git merge release_@MAJOR@@MINOR@
# Resolve any conflicts, then:
# Be very careful here
git push origin master
```
<!-- endif -->
- [ ] Submit a PR in Spack that adds this new version of ADIOS (if not RC mark this new version as preferred)
  - Run `spack checksum -a adios2` to add it, create commit; push it; Create
    PR in Spack repo.
- [ ] Submit a PR in Conda that adds this new version of ADIOS (if not RC mark this new version as preferred)
  - CondaForge robot should do this for you automatically, expect a new PR at
    https://github.com/conda-forge/adios2-feedstock a couple of hours after the
    release.
- [ ] Submit a MR for ParaView Superbuild to use v@VERSION@ release.
- [ ] Update the website to point to the v@VERSION@ release
- [ ] Write an announcement in the ADIOS-ECP mail-list
  (https://groups.google.com/a/kitware.com/g/adios-ecp)

