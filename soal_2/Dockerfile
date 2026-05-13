FROM ubuntu:latest

# Set working directory
WORKDIR /app

# Copy server binary ke dalam container
COPY server /app/server

# Pastikan file server dapat dieksekusi
RUN chmod +x /app/server

# Buat folder db yang akan di-bind dengan fuse_mount
RUN mkdir -p /app/db

# Expose port TCP yang digunakan oleh MOO Database
EXPOSE 9000

# Jalankan server MOO
CMD ["./server"]