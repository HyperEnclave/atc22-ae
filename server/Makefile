TAG ?= 0.1.0-ubuntu20.04
HU ?=

ifeq ($(HU),)
    dockerfile := Dockerfile
    image_name := equation314/hyperenclave-ae-server-image:$(TAG)
    container_name := hyperenclave-ae-server-$(shell whoami)
else
    dockerfile := Dockerfile.hu
    image_name := equation314/hyperenclave-ae-server-image-hu:$(TAG)
    container_name := hyperenclave-ae-server-hu-$(shell whoami)
endif

args := --net=host \
        --device=/dev/hyper_enclave \
        --privileged \
        -w /home/admin/dev \
        --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
        --cap-add=NET_ADMIN \

build:
	git submodule update --init --recursive
	docker build -f $(dockerfile) --network=host -t $(image_name) .

start:
	sudo docker run -dt --name $(container_name) $(args) $(image_name)

enter:
	sudo docker start $(container_name)
	sudo docker exec -it $(container_name) bash

push:
	docker push $(image_name)

pull:
	docker pull $(image_name)
