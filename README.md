Moved to https://git.opendlv.org.

# opendlv-ui-videostream

This repository provides the source code to hook into a shared memory video
stream and send it over a conference. The image will be encoded in JPEG format,
and sent as the ImageReading message from the Standard message set.

## Usage
This microservice is created automatically on new releases, via Docker's public registry:

* [x86_64](https://hub.docker.com/r/chalmersrevere/opendlv-ui-videostream-amd64/tags/)
* [armhf](https://hub.docker.com/r/chalmersrevere/opendlv-ui-videostream-armhf/tags/)

To run this microservice using our pre-built Docker multi-arch images, 
simply start it as:

```
docker run \
           --rm \
           -ti \
           --init \
           -v /dev/shm:/dev/shm \
           --ulimit memlock=4000000:4000000 \
           opendlv-ui-videostream \
               --cid=111 \
               --freq=5
               --name=camera0 \
               --width=1024 \
               --height=768 \
               --bpp=24 \
               --scaled-width=256
               --scaled-height=192
```

## Build from sources
To build this software, you need cmake, C++14 or newer, and make. Having these
preconditions, just run `cmake` and `make` as follows:

```
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make && make install
```


## License

* This project is released under the terms of the GNU GPLv3 License
