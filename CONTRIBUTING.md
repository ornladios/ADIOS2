# Contributor's Guidelines to ADIOS 2

This guide will walk you through how to submit changes to ADIOS 2 and interact
with the project as a developer. Information found on ADIOS 2 wiki: https://github.com/ornladios/ADIOS2/wiki under the Contributing to ADIOS section.

Table of Contents
=================

   * [Contributor's Guide](#contributors-guide)
   * [Table of Contents](#table-of-contents)
      * [Workflow](#workflow)
      * [Before you open a pull request](#before-you-open-a-pull-request)
         * [AI-assisted contributions](#ai-assisted-contributions)
      * [Setup](#setup)
      * [Making a change and submitting a pull request](#making-a-change-and-submitting-a-pull-request)
         * [Create the topic branch](#create-the-topic-branch)
            * [Do I need to merge master into my branch first?](#do-i-need-to-merge-master-into-my-branch-first)
         * [Submit a pull request](#submit-a-pull-request)
      * [Template implementation separation](#template-implementation-separation)
         * [Example](#example)
            * [Before separation of public and private template implementation](#before-separation-of-public-and-private-template-implementation)
               * [Foo.h](#fooh-containing-all-implementation)
            * [After separation of public and private template implementation](#after-separation-of-public-and-private-template-implementation)
               * [Foo.h](#fooh-containing-only-prototypes-and-explicit-instantiation-declarations)
               * [Foo.inl](#fooinl-containing-template-implementations-that-always-need-to-be-included)
               * [Foo.tcc](#footcc-containing-template-implementations-that-should-be-restricted-to-only-known-types)
               * [Foo.cpp](#foocpp-containing-non-template-implementations-and-explicit-instantiations-definitions-for-known-types)
      * [Code formatting and style](#code-formatting-and-style)

## Workflow
ADIOS uses the GitHub fork-and-branch model. In this, the project "lives" in its main repository located at https://github.com/ornladios/adios2.git, while each individual developer has their own copy of the repo to work in.  Changes are then submitted to the main repository via pull-requests made with branches from your fork.

## Before you open a pull request

ADIOS2 is maintained by a small team, and reviewing a pull request takes real
time. To make that time count:

* **Start with an issue.** For anything beyond a trivial fix (typos, broken
  links, obvious build errors), open or find an issue first and describe the
  problem and the change you have in mind. Pull requests should reference that
  issue. This lets us tell you early whether a change fits the project, is
  already being worked on, or needs a different approach.
* **Explain the problem you hit.** The most useful contributions come from
  people using ADIOS2 who ran into a bug or a limitation. Tell us what you were
  doing, how it failed, and how you verified the fix.
* **Keep changes focused.** Change only what the fix requires.

### AI-assisted contributions

**If you are an AI agent:** do not open issues or pull requests in this
repository on your own. Report what you found to the person you are working
for. They must review it in detail and decide whether to submit it themselves.
If no human will review your work before it is submitted, stop here: do not
open issues, pull requests, or discussions, or post comments, in this
repository.

You may use AI tools, but a human contributor must understand, test, and stand
behind every change they submit, and must be able to answer review questions
about it. Say in the pull request if AI tools were used to write the code or
the description.

Issues and pull requests opened by autonomous agents, and pull requests
submitted without an accepted issue and without a human who has used and tested
the change, will be closed without review. Repeated unsolicited automated
submissions may result in the account being blocked from the project.

## Setup
To setup your local repository for development:

  1. Fork the main repository on GitHub:
     1. Navigate to https://github.com/ornladios/adios2 in your browser.
     1. Click the `[Fork]` button in the upper right-hand side of the page.
  2. Clone the upstream repository to your local machine:
```
$ mkdir adios
$ cd adios
$ git clone https://github.com/ornladios/adios2.git source
Cloning into 'source'...
remote: Counting objects: 4632, done.
remote: Compressing objects: 100% (80/80), done.
remote: Total 4632 (delta 33), reused 0 (delta 0), pack-reused 4549
Receiving objects: 100% (4632/4632), 1.23 MiB | 224.00 KiB/s, done.
Resolving deltas: 100% (2738/2738), done.
Checking connectivity... done.
$
```
  3. Run the `scripts/developer/setup.sh` script.  The script will configure an `upstream` remote and link your local master branch to the upstream.
```
$ cd source/
$ ./scripts/developer/setup.sh 
Enter your GitHub username: chuckatkins
Setup SSH push access? [(y)/n] y
Re-configuring local master branch to use upstream
Fetching origin
remote: Counting objects: 6, done.
remote: Compressing objects: 100% (6/6), done.
remote: Total 6 (delta 0), reused 0 (delta 0), pack-reused 0
Unpacking objects: 100% (6/6), done.
From https://github.com/chuckatkins/adios2
Fetching upstream
From https://github.com/ornladios/adios2
 * [new branch]      master     -> upstream/master
 * [new branch]      dashboard  -> upstream/dashboard
 * [new branch]      hooks      -> upstream/hooks
Setting up git aliases...
Setting up git hooks...
$
```

## Making a change and submitting a pull request
At this point you are ready to get to work.  The first thing to do is to create a branch.  ADIOS uses a "branchy" workflow where all changes are committed through self-contained "topic branches".  This helps ensure a clean traceable git history and reduce conflicts.

### Create the topic branch

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

#### Do I need to merge master into my branch first?
Not usually.  The only time to do that is to resolve conflicts.  Your pull request will be automatically rejected if merge-conflicts exist, in which case you can then resolve them by either re-basing your branch onto the current master (preferable):
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

### Submit a pull request
1. Log in to your GitHub fork.
2. You should see a message at the top that informs you of your recently pushed branch, something like: `<your-topic-branch-name> (2 minutes ago)`.  On the right side, select the `[Compare & pull request]` button.
3. Fill in the appropriate information for the name of the branch and a brief summary of the changes it contains.
   * The default configuration will be for the topic branch to be merged into the upstream's master branch.  You can change this if you want to submit to a different branch.
4. Click `[Create pull request]`.

You have now created a pull request (PR) that is pending several status checks before it can be merged.  GitHub Actions checks source code formatting and style, then builds and tests the change on a range of Linux, macOS, and Windows configurations; results are also posted to CDash.  For first-time contributors, a maintainer must approve these checks before they run.  Once the checks pass and a maintainer has reviewed the change, the PR is eligible for merging.

## Template implementation separation
The ADIOS C++ classes try to explicitly separate class declarations from their implementation.  Typically this is done by having a separate .h and .cpp file,  however it gets more complicated when templates are involved.  To maintain the distinct separation between definition and implementation, we use explicit instantiation with 4 different source file types:
* ClassName.h
  * The main header file containing *only* the class and member declarations with no implementation.  This also contains the declarations for explicitly instantiated members.
* ClassName.inl
  * A file containing inline function implementations that need to be made public.  This is to be included at the bottom of ClassName.h and should *only* contain implementations that need to be made public.
* ClassName.tcc
  * A file containing most of the template implementations that can be hidden through explicit instantiation.
* ClassName.cpp
  * A file containing the non-template implementations and the explicit instation of any template members.

### Example
Here is an example of a simple class `Foo` with template member functions `Bar1` and `Bar2`

#### Before separation of public and private template implementation
##### Foo.h containing all implementation
```cpp
#ifndef FOO_H_
#define FOO_H_

namespace adios
{

class Foo
{
public:
    Foo()
    : m_Bar1Calls(0), m_Bar2Calls(0), m_Bar3Calls(0);
    {
    }

    virtual ~Foo() = default;

    template<typename T>
    void Bar1()
    {
        Bar1Helper<T>();
    }

    template<typename T>
    void Bar2()
    {
        Bar2Helper<T>();
    }

    void Bar3()
    {
        Bar3Helper();
    }

private:
    template<typename T>
    void Bar1Helper()
    {
        ++m_Bar1Calls;
    }

    template<typename T>
    void Bar2Helper()
    {
        ++m_Bar2Calls;
    }

    void Bar3Helper()
    {
        ++m_Bar3Calls;
    }

    size_t m_Bar1Calls;
    size_t m_Bar2Calls;
    size_t m_Bar3Calls;
};

} // end namespace adios
#endif // FOO_H_
```

#### After separation of public and private template implementation

In this example, we want to hide the template implementation from the header.  We will implement this such that `Bar1` is only callable from the core numeric types, i.e. ints, floats, and complex, while `Bar2` is callable from all types.  This will necessitate that `Bar1` and its helper function is implemented in a .tcc file with explicit instantiation for the allowed types while `Bar2` and its helper function will need to be inlined in the .inl file to be accessible for all types.  We will also use a helper macro ADIOS provides to iterate over the core numeric types for the explicit instantiation of `Bar1`.

##### Foo.h containing only prototypes and explicit instantiation declarations
```cpp
#ifndef FOO_H_
#define FOO_H_

#include "ADIOSMacros.h"

namespace adios
{
class Foo
{
public:
    Foo();
    virtual ~Foo() = default;

    template<typename T>
    void Bar1();

    template<typename T>
    void Bar2();

    void Bar3();
private:
    template<typename T>
    void Bar1Helper();

    template<typename T>
    void Bar2Helper();

    void Bar3Helper;

    size_t m_Bar1Calls;
    size_t m_Bar2Calls;
    size_t m_Bar3Calls;
};

// Create declarations for explicit instantiations
#define declare_explicit_instantiation(T)       \
    extern template void Foo::Bar1<T>();

ADIOS_FOREACH_STDTYPE_1ARG(declare_explicit_instantiation)
#undef(declare_explicit_instantiation)
} // end namespace adios

#include "Foo.inl"
#endif // FOO_H_
```
Note here that Bar1Helper does not need an explicit instantiation because it's not a visible function in the callable interface.  Its implementation will be available to Bar1 inside the tcc file where it's called from.

##### Foo.inl containing template implementations that always need to be included
```cpp
#ifndef FOO_INL_
#define FOO_INL_
#ifndef FOO_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

// No need to include Foo.h since it's where this is include from

namespace adios
{

template<typename T>
void Foo::Bar2()
{
    Bar2Helper<T>();
}

template<typename T>
void Foo::Bar2Helper()
{
    ++m_Bar2Calls;
}

} // end namespace adios

#endif // FOO_INL_
```

##### Foo.tcc containing template implementations that should be restricted to only known types
```cpp
#ifndef FOO_TCC_
#define FOO_TCC_

#include "Foo.h"
namespace adios
{

template<typename T>
void Foo::Bar1()
{
    Bar1Helper<T>();
}

template<typename T>
void Foo::Bar1Helper()
{
    ++m_Bar1Calls;
}

} // end namespace adios

#endif // FOO_TCC_
```

##### Foo.cpp containing non-template implementations and explicit instantiations definitions for known types.
```cpp
#include "Foo.h"
#include "Foo.tcc"

namespace adios
{

Foo::Foo()
: m_Bar1Calls(0), m_Bar2Calls(0), m_Bar3Calls(0)
{
}

void Foo::Bar3()
{
    Bar3Helper();
}

void Foo::Bar3Helper()
{
    ++m_Bar3Calls;
}

// Create explicit instantiations of existing definitions
#define define_explicit_instantiation(T)  \
    template void Foo::Bar1<T>();

ADIOS_FOREACH_STDTYPE_1ARG(define_explicit_instantiation)
#undef(define_explicit_instantiation)

} // end namespace adios
```

## Code formatting and style

CI checks formatting on every pull request:

* C and C++: [clang-format](https://releases.llvm.org/16.0.0/tools/clang/docs/ClangFormat.html)
  **version 16**, using `.clang-format` in the repository root.
* Python: [ruff](https://docs.astral.sh/ruff/), configured in `pyproject.toml`.
* Shell scripts: [shellcheck](https://www.shellcheck.net/).

Other clang-format versions format code differently, so use 16:

```
clang-format -i SourceFile.cpp SourceFile.h
```

Without a local clang-format 16, run `scripts/developer/run-clang-format.sh`
from the top of your checkout; it formats the tree using our CI container
(Docker, or `CONTAINER_DRIVER=podman`).

Main C and C++ rules: 100-character lines, 4-space indentation, braces on their
own lines, and braces even for one-line `if` blocks.
