FROM ubuntu:20.04

RUN apt update && \
    DEBIAN_FRONTEND="noninteractive" apt upgrade -y --no-install-recommends && \
    DEBIAN_FRONTEND="noninteractive" apt install -y --no-install-recommends \
    apt-utils \
    ca-certificates \
    clang-format-7 \
    curl \
    flake8 \
    git \
    libtinfo5 \
    shellcheck \
    && \
    apt-get clean
