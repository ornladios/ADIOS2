# Compile a fully static node.js binary for use by github actions
FROM alpine:3.11.3
RUN apk add git python gcc g++ linux-headers make
ARG TAG=v12.16.3
RUN git clone -b ${TAG} --single-branch --depth 1 https://github.com/nodejs/node && \
        cd node && \
        ./configure --fully-static --enable-static && \
        make -j$(grep -c '^processor' /proc/cpuinfo)

FROM scratch
COPY --from=0 /node/out/Release/node /node
CMD /node
