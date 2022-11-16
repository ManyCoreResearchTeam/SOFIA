#!/usr/bin/env sh
docker build \
    --build-arg "my_user=${USER}" \
    --build-arg "my_uid=501" \
    --build-arg "my_gid=20" \
    -f Dockerfile \
    -t sofia-gem5:latest .
