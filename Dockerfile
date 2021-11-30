FROM ubuntu:20.04 as builder

RUN apt update && \
    apt install -y build-essential libncurses-dev

ADD . /app
WORKDIR /app
RUN make clean && make

FROM ubuntu:20.04
COPY --from=builder /app/snek /bin/snek