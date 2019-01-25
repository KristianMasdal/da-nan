#! /bin/bash

docker_image_id=$(docker images | grep nan | tr -s ' '  | cut -f3 -d " ")
docker_container_id=$(docker ps -a | grep server | cut -f1 -d " ")

docker rm -f $docker_container_id
docker rmi $docker_image_id
