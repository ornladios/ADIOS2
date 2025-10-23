from python:3.14.0

# run apt install without asking user
RUN DEBIAN_FRONTEND=noninteractive apt update && \
    apt -y install --no-install-recommends ssh git && \
    pip install python-dateutil PyGithub
