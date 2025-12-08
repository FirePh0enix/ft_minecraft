FROM alpine:3.22.2

RUN apk update && apk add \
    bash git python3 zip \
    lldb clang-dev \
    cmake make ninja clang clang-libclang musl-dev linux-headers \
    cargo rust \
    libx11-static libxext-static libx11-dev libxext-dev

WORKDIR /data
