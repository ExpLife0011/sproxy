# This dockerfile uses the debian:sid image

# Base image to use, this must be set as the first line
FROM alpine

# Maintainer: docker_user <docker_user at email.com> (@docker_user)
MAINTAINER choury zhouwei400@gmail.com

# Commands to update the image
RUN apk update && \
    apk add gcc g++ cmake make wget libexecinfo-dev openssl-dev libgcc libstdc++ ca-certificates && \
    cd /root && \
    wget "https://github.com/choury/sproxy/archive/master.zip" && \
    unzip master.zip && \
    rm master.zip && \
    cd /root/sproxy-master && \
    cmake . && \
    make sproxy VERBOSE=1 && \
    wget https://gist.githubusercontent.com/choury/c42dd14f1f1bfb9401b5f2b4986cb9a9/raw/sites.list && \
    apk del gcc g++ cmake make wget
#COPY keys /root/keys/

# Commands when creating a new container
EXPOSE 80
WORKDIR /root/sproxy-master
ENTRYPOINT ["./sproxy"]
