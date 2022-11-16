#!/usr/bin/env sh
#docker run -it --rm -v $(pwd):/mnt:cached -w /mnt --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --privileged sofia-gem5:latest
docker run -it --rm -v $(pwd):/mnt:cached -w /mnt --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --privileged -u root sofia-gem5:latest
