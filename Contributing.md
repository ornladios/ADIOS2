# Contributor's Guide

This guide will walk you through how to submit changes to ADIOS and interact
with the project as a developer.

## Workflow
ADIOS uses the GitHub fork-and-branch model. In this, the project "lives" in it's main repository located at https://github.com/ornladios/adios2.git, while each individual developer has their own copy of the repo to work in.  Changes are then submitted to the main repository via pull-requests made with branches from your fork.

### Setup
To setup your local repository for development:

  1. Fork the main repository on GitHub:
     1. Navigate to https://github.com/ornladios/adios2 in your browser.
     1. Click the `[Fork]` button in the upper right-hand side of the page.
  2. Clone your fork to your local machine:
```
[chuck@hal9000 Code]$ mkdir adios
[chuck@hal9000 Code]$ cd adios
#
# If you don't have ssh access setup to your github repository then you will
# need to clone your fork from
# https://github.com/<your-GitHub-username-here>/adios2.git
# instead of
# git@github.com:<your-GitHub-username-here>/adios2.git
#
[chuck@hal9000 adios]$ git clone git@github.com:<your-GitHub-username-here>/adios2.git source
Cloning into 'source'...
remote: Counting objects: 4535, done.
remote: Compressing objects: 100% (223/223), done.
remote: Total 4535 (delta 64), reused 0 (delta 0), pack-reused 4301
Receiving objects: 100% (4535/4535), 1.27 MiB | 0 bytes/s, done.
Resolving deltas: 100% (2607/2607), done.
Checking connectivity... done.
[chuck.atkins@hal9000 adios]$
```
  3. Run the `scripts/development/setup.sh` script.  The script will configure an `upstream` remote and link your local master branch to the upstream.
```
[chuck.atkins@hal9000 adios]$ cd source/
$ ./scripts/developer/setup.sh
Checking GitHub username...
Using GitHub user <your-GitHub-username-here>
Adding upstream remote...
Fetching origin
Fetching upstream
From https://github.com/ornladios/adios2
 * [new branch]      dashboard  -> upstream/dashboard
 * [new branch]      hooks      -> upstream/hooks
 * [new branch]      master     -> upstream/master
Re-configuring local master branch to use upstream
Setting up git aliases...
Setting up git hooks...
$
```

### Making a change and submitting a pull request
At this point you are ready to get to work.  The first thing to do is to create a branch.  ADIOS uses a "branchy" workflow where all changes are committed through self-contained "topic branches".  This helps ensure a clean traceable git history and reduce conflicts.

#### Create the topic branch

1. Make sure you are starting from a current master:
```
$ git checkout master
$ git pull
```
2. Create a branch for your change:
```
$ git checkout -b <your-topic-branch-name>
```
3. Make your changes and commits to the branch.
4. Push the branch to your fork:
```
$ git push -u origin HEAD
Counting objects: 189, done.
Delta compression using up to 8 threads.
Compressing objects: 100% (134/134), done.
Writing objects: 100% (189/189), 70.30 KiB | 0 bytes/s, done.
Total 189 (delta 128), reused 102 (delta 44)
remote: Resolving deltas: 100% (128/128), completed with 82 local objects.
To git@github.com:<your-GitHub-username-here>/adios2.git
 * [new branch]      HEAD -> <your-topic-branch-name>
Branch <your-topic-branch-name> set up to track remote branch <your-topic-branch-name> from origin.
$
```

##### Do I need to merge master into my branch first?
Not usually.  The only time to do that is to resolve conflicts.  You're pull request will be automatically rejected if merge-conflicts exist, in which case you can then resolve them by either re-basing your branch the current master (preferable):
```
$ git fetch --all -p
...
$ git rebase upstream/master
$ git push -f
```
or if necessary or re-basing is not a viable option then you can always fall back to merging in master but it should be generally discouraged as it tends to make the git history difficult to follow:
```
$ git fetch --all -p
...
$ git merge upstream/master
$ git push -f
```

#### Submit a pull request
1. Log in to your GitHub fork.
2. You should see a message at the top that informs you of your recently pushed branch, something like: `<your-topic-branch-name> (2 minutes ago)`.  On the right side, select the `[Compare & pull request]` button.
3. Fill in the appropriate information for the name of the branch and a brief summary of the changes it contains.
   * The default configuration will be for the topic branch to be merged into the upstream's master branch.  You can change this if you want to submit to a different branch.
4. Click `[Create pull request]`.

You have now created a pull request (PR) that is pending several status checks before it can be merged.  Currently, the only check being performed is for source code formatting and style.  In the future, however, the will be a more in depth continuous integration system tied to the pull requests that tests for build and test failures every time a PR is submitted or updated.  Once the status checks pass, the PR will be eligible for merging by one of the project maintainers.

### Code formatting and style
ADIOS uses the clang-format tool to automatically enforce source code style and formatting rules.  There are various ways to integrate the clang-format tool into your IDE / Code Editor depending on if you use Emacs, Vim, Eclipse, KDevelop, Microsoft Visual Studio, etc. that are a bit outside the scope of this document but a quick google search for "integrate <insert-editor-here> clang-format" should point you in the right direction.  However, you can always reformat the code manually by running:
```
clang-format -i SourceFile.cpp SourceFile.h
```
That will apply the formatting rules used by the ADIOS project.
1. Lines no longer than 80 characters.
1. Always use braces { and }, even for 1 line if blocks.
1. Use 4 spaces for indentation.
There are more formatting rules but these three should at least get you close and prevent any drastic re-writes from the re-formatting tools.  More details can be found by looking at the .clang-format config file n the root of the repository and by looking at the clang-format documentation http://releases.llvm.org/3.8.0/tools/clang/docs/ClangFormatStyleOptions.html.  While the formatting rules are a bit more involved, the main points are:
