#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error: Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Error: Invalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error: Connection Failed \n");
        return -1;
    }

    // Header
    printf("Connected to DB Server on port %d\n", PORT);
    printf("Type HELP for available commands\n");
    printf("Type EXIT to quit\n\n");

    while (1) {
        printf("db > ");
        memset(input, 0, BUFFER_SIZE);
        
        // Membaca input dengan fgets (otomatis simpan \n di akhir string)
        if (fgets(input, BUFFER_SIZE, stdin) != NULL) {
            
            // Cek perintah EXIT
            if (strncmp(input, "EXIT", 4) == 0) {
                break;
            }

            // Kirim input utuh beserta \n ke server
            send(sock, input, strlen(input), 0);
            
            // Tunggu dan baca balasan dari server
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = read(sock, buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                printf("\n%s\n", buffer);
            }
        }
    }

    close(sock);
    return 0;
}