#! /bin/bash
# Run as root

gcc -static -o server server.c

systemctl stop docker
dockerd --userns-remap=webserver:webserver &

docker build -t nan/mp1 .
docker run --cap-drop=ALL --cap-add=NET_BIND_SERVICE --cap-add=SETUID --cap-add=SETGID --cap-add=SYS_CHROOT -p 80:80 --name server --cpuset-cpus 0 --pids-limit 200 nan/mp1 &

#--cap-drop=ALL --cap-add=NET_BIND_SERVICE
#SETUID SETGID SYS_CHROOT
