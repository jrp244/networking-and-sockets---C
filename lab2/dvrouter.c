#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define DV_PORT 5016
#define BUFFER_SIZE 65536
#define NEIGHBOR_CHECK_INTERVAL 3
#define DV_TABLE_SEND_INTERVAL 1

typedef struct {
    char prefix[32];
    int distance;
} DVEntry;

typedef struct {
    char ip[16];
    char name[32];
    DVEntry dv_table[100];
    int dv_count;
} NeighborDV;

NeighborDV neighbors[10];
int neighbor_count = 0;
DVEntry my_dv_table[100];
int my_dv_count = 0;

void initialize_socket(int *sock) {
    *sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int broadcast = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("Failed to set broadcast option");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DV_PORT);

    if (bind(*sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }
}

void send_dv(int sock) {
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(DV_PORT);
    inet_pton(AF_INET, "255.255.255.255", &broadcast_addr.sin_addr);

    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "{\"ip\": \"192.168.1.1\", \"name\": \"router1\", \"dv\": [");

    for (int i = 0; i < my_dv_count; i++) {
        char entry[64];
        snprintf(entry, sizeof(entry), "{\"prefix\": \"%s\", \"distance\": %d}", my_dv_table[i].prefix, my_dv_table[i].distance);
        strcat(message, entry);
        if (i < my_dv_count - 1) strcat(message, ", ");
    }
    
    strcat(message, "]}");

    if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("Failed to send message");
    }
}

void update_dv() {
    // Simplified distance vector update logic
    for (int i = 0; i < neighbor_count; i++) {
        for (int j = 0; j < neighbors[i].dv_count; j++) {
            int found = 0;
            for (int k = 0; k < my_dv_count; k++) {
                if (strcmp(my_dv_table[k].prefix, neighbors[i].dv_table[j].prefix) == 0) {
                    found = 1;
                    int new_distance = neighbors[i].dv_table[j].distance + 1;
                    if (new_distance < my_dv_table[k].distance) {
                        my_dv_table[k].distance = new_distance;
                    }
                }
            }
            if (!found) {
                strcpy(my_dv_table[my_dv_count].prefix, neighbors[i].dv_table[j].prefix);
                my_dv_table[my_dv_count].distance = neighbors[i].dv_table[j].distance + 1;
                my_dv_count++;
            }
        }
    }
}

void handle_message(int sock) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    int len = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (len > 0) {
        buffer[len] = '\0';
        printf("Received message: %s\n", buffer);

        // Parse received DV table (simplified parsing)
        // In practice, use a JSON library like cJSON to parse the message.
        // Here we assume the message format is known and skip detailed parsing.
        
        // Update neighbor DV table and update our own DV table
        update_dv();
    }
}

int main() {
    int sock;
    initialize_socket(&sock);

    // Initialize local DV table
    strcpy(my_dv_table[0].prefix, "192.168.1.1/32");
    my_dv_table[0].distance = 0;
    my_dv_count++;

    time_t last_send_time = time(NULL);
    
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = DV_TABLE_SEND_INTERVAL;
        timeout.tv_usec = 0;

        int activity = select(sock + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(sock, &read_fds)) {
            handle_message(sock);
        }

        time_t current_time = time(NULL);
        if (current_time - last_send_time >= DV_TABLE_SEND_INTERVAL) {
            send_dv(sock);
            last_send_time = current_time;
        }
        
        // Periodically update DV table
        update_dv();
    }

    close(sock);
    return 0;
}
