# Sample
FROM ubuntu:21.04

ENV DEBCONF_NOWARNINGS yes 
ENV DEBIAN_FRONTEND = noninteractive

RUN apt-get update && apt-get autoremove && apt-get clean

# GNU C++ 10
RUN apt-get update && apt-get install -y build-essential g++-10 gdb && \
    apt-get autoremove && apt-get clean

# Python 3.9 (For local emulator)
RUN apt-get update && apt-get install -y python3.9 python3-pip && \
    ln -sf /usr/bin/python3 /usr/bin/python && \
    ln -sf /usr/bin/pip3 /usr/bin/pip && \
    apt-get autoremove && apt-get clean

COPY . /code
