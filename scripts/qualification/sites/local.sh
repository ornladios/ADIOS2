# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Sourced by qualify.sh, which reads these settings and sets site, cmd, ROOT and SELF.
# shellcheck shell=bash disable=SC1091,SC2034,SC2154

# Any MPI workstation build, for trying out changes to these scripts.
# Set QUAL_BUILD to an existing build tree to skip the build step.
SITE_ROOT="${TMPDIR:-/tmp}/adios2-qualification"
DATAPLANES="evpath"
CTEST_J=4

site_env() { :; }
site_submit() { "$SELF" "$site" run; }
