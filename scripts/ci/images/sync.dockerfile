# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

from python:3.6.15

# run apt install without asking user
RUN DEBIAN_FRONTEND=noninteractive apt update && \
    apt -y install --no-install-recommends ssh git && \
    pip install python-dateutil PyGithub
