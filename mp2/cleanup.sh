#! /bin/bash

# Run as root

docker rm -f $(docker ps -aq)
docker rmi $(docker images -q)
docker volume rm $(docker volume ls -q)
killall dockerd -v
systemctl start docker

