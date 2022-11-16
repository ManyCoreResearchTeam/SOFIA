APPS =
DIR = $(shell pwd)
OVP_V =
TAG_NAME = mpsoc/sofia-ovp:latest
V =

ifeq (V, 1)
	V=
else
	V=@
endif

.PHONY: all build shell

all : build shell

build:
	$(V)docker build --build-arg my_user=$${USER} --build-arg USER_UID=$(shell id -u) --build-arg USER_GID=$(shell id -g) --build-arg USER_NAME=$(shell id -un) -t $(TAG_NAME) - < Dockerfile

shell :
	$(V)make --no-print-directory docker OVP_V=$(OVP_V) CMD="/bin/bash -i"

docker :
	$(V)docker run -it --rm \
		-v $(DIR):/root/ \
		-v /soft64/:/soft64/ \
		$(TAG_NAME) $(CMD)
