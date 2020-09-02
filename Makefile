# shortcuts for my own use

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  NCPU := $(shell grep -c '^processor' /proc/cpuinfo)
endif

REMOTE_PREFIX = xysperi
IMAGE = dev-ubuntu
TAG = latest
NAME = dev-ubuntu
.PHONY: docker-run
docker-run:
	docker run --name $(NAME) \
	  --rm -it \
	  --workdir /workspace \
	  -v $(PWD):/workspace \
	  $(REMOTE_PREFIX)/$(IMAGE):$(TAG) \
	  /bin/bash

.PHONY: clean
clean:
	rm -rf _build/

.PHONY: cmake-gen
cmake-gen:
	mkdir -p _build/ && cd _build/ && cmake .. && cd ..

.PHONY: build
build:
	cd _build/ && make -j $(NCPU) && cd ..
