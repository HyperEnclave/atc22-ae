TAG ?= 0.1.0-ubuntu20.04

image_name := equation314/hyperenclave-ae-client-image:$(TAG)
container_name := hyperenclave-ae-client-$(shell whoami)

args := --net=host

build:
	docker build --network=host -t $(image_name) .

start:
	sudo docker run -dt --name $(container_name) $(args) $(image_name)

enter:
	sudo docker start $(container_name)
	sudo docker exec -it $(container_name) bash

push:
	docker push $(image_name)

pull:
	docker pull $(image_name)
