FROM scratch
VOLUME /var/
VOLUME /var/www
COPY server /usr/server/
COPY /www/ /var/www/
EXPOSE 80
ENTRYPOINT ["/usr/server/server"]
