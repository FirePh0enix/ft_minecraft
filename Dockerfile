FROM ubuntu:24.04

WORKDIR /data
RUN apt update && apt install -y build-essential cmake wget git python3 \
    libx11-dev libxext-dev
RUN wget -qO /usr/local/bin/ninja.gz https://github.com/ninja-build/ninja/releases/latest/download/ninja-linux.zip && gunzip /usr/local/bin/ninja.gz && chmod a+x /usr/local/bin/ninja
