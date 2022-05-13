FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update; \
    apt-get install -y \
        apache2-utils \
        openjdk-8-jre \
        redis-tools \
        wget

WORKDIR /root

RUN mkdir tools
RUN cd tools && \
    wget https://github.com/brianfrankcooper/YCSB/releases/download/0.17.0/ycsb-redis-binding-0.17.0.tar.gz && \
    tar -zxf ycsb-redis-binding-0.17.0.tar.gz

# Copy benchmarks
COPY lighttpd /root/lighttpd
COPY redis /root/redis
