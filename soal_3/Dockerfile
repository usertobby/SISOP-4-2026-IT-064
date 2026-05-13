FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    samba \
    samba-common-bin \
    && rm -rf /var/lib/apt/lists/*

# Buat direktori data
RUN mkdir -p /data/ebooks \
             /data/papers \
             /data/sourcecode \
             /data/docs

# Copy konfigurasi Samba
COPY smb.conf /etc/samba/smb.conf

# Copy entrypoint
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 445 139

ENTRYPOINT ["/entrypoint.sh"]