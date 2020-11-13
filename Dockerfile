FROM adnrv/opencv:3.4

RUN apt-get update -y && apt-get clean -y

COPY . /stream_files

RUN cd /stream_files/seasocks && mkdir build && cd build && cmake .. && make

RUN cd /stream_files && ./build.sh && chmod ugo+x streamer

WORKDIR /stream_files

ENTRYPOINT ["./streamer"] 


