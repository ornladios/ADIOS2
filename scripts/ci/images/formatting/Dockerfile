FROM ubuntu:20.04

RUN apt-get update && \
    apt-get install -y apt-utils && \
    apt-get install -y curl git flake8 libtinfo5 && \
    apt-get clean

RUN curl -L https://github.com/llvm/llvm-project/releases/download/llvmorg-7.1.0/clang+llvm-7.1.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz | tar -C /opt -xJv && \
    mv /opt/clang* /opt/llvm-7.1.0
ENV PATH=/opt/llvm-7.1.0/bin:$PATH
