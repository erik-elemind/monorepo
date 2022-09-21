FROM alpine:3.16
RUN apk update
RUN apk add --no-cache git
RUN apk add --no-cache make
RUN apk add --no-cache curl
RUN apk add --no-cache newlib-arm-none-eabi

# Install python/pip
RUN apk add --no-cache python3
RUN apk add --no-cache py3-pip

WORKDIR /usr/project
CMD make dist;make sdk;make V=1 app;exit