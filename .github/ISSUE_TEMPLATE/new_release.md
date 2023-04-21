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
- [ ] Create Tag commit `git tag -a v@VERSION@ the_merge_commit`
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
git push origin
```
<!-- else -->
- [ ] Create PR that merges release_@MAJOR@@MINOR@ into master
- [ ] Submit a PR in Spack that adds this new version of ADIOS (if not RC mark this new version as preferred)
- [ ] Write an announcement in the ADIOS-ECP mail-list
  (https://groups.google.com/a/kitware.com/g/adios-ecp)
