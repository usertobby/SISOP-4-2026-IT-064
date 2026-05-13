#!/bin/bash
set -e

# Buat grup
groupadd -f readonly
groupadd -f staff

# Buat user sistem + tambahkan ke grup
create_user() {
    local USERNAME=$1
    local PASSWORD=$2
    local GROUP=$3

    if ! id "$USERNAME" &>/dev/null; then
        useradd -M -s /sbin/nologin "$USERNAME"
    fi
    echo "$USERNAME:$PASSWORD" | chpasswd
    usermod -aG "$GROUP" "$USERNAME"

    # Daftarkan ke Samba
    (echo "$PASSWORD"; echo "$PASSWORD") | smbpasswd -s -a "$USERNAME"
    smbpasswd -e "$USERNAME"
}

create_user "member"      "member123"  "readonly"
create_user "contributor" "contrib456" "staff"
create_user "librarian"   "lib789"     "staff"

# Set kepemilikan & permission direktori

# ebooks: staff rw, readonly r
chown root:staff /data/ebooks
chmod 775 /data/ebooks

# papers: staff rw, readonly r
chown root:staff /data/papers
chmod 775 /data/papers

# sourcecode: hanya owner+grup staff, others tidak bisa sama sekali
chown root:staff /data/sourcecode
chmod 770 /data/sourcecode

# docs: chown/chmod dilakukan oleh service 'setup' di docker-compose
# karena di sini di-mount :ro — tidak bisa dimodifikasi dari dalam container

# Setup log file
LOG_FILE="/var/log/samba/libraryit.log"
mkdir -p /var/log/samba
touch "$LOG_FILE"
chmod 644 "$LOG_FILE"

# Jalankan Samba di foreground
exec smbd --foreground --no-process-group --configfile=/etc/samba/smb.conf