#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>

#define MAC_TABLE_SIZE 256
#define AGING_TIME 8 // seconds

typedef struct mac_entry {
    unsigned char mac[6]; // MAC address
    char interface[16];   // Interface name
    time_t timestamp;     // Last seen time
    struct mac_entry *next;
} mac_entry_t;

typedef struct switch_t {
    mac_entry_t *mac_table[MAC_TABLE_SIZE];
} switch_t;

unsigned int hash_mac(const unsigned char *mac) {
    unsigned int hash = 0;
    for (int i = 0; i < 6; i++) {
        hash = (hash * 31) + mac[i];
    }
    return hash % MAC_TABLE_SIZE;
}

void add_mac_entry(switch_t *sw, const unsigned char *mac, const char *interface) {
    unsigned int index = hash_mac(mac);
    mac_entry_t *entry = malloc(sizeof(mac_entry_t));
    memcpy(entry->mac, mac, 6);
    strncpy(entry->interface, interface, sizeof(entry->interface));
    entry->timestamp = time(NULL);
    entry->next = sw->mac_table[index];
    sw->mac_table[index] = entry;
}

mac_entry_t *find_mac_entry(switch_t *sw, const unsigned char *mac) {
    unsigned int index = hash_mac(mac);
    mac_entry_t *entry = sw->mac_table[index];
    while (entry) {
        if (memcmp(entry->mac, mac, 6) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

void remove_expired_entries(switch_t *sw) {
    time_t now = time(NULL);
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        mac_entry_t **entry_ptr = &sw->mac_table[i];
        while (*entry_ptr) {
            if (now - (*entry_ptr)->timestamp > AGING_TIME) {
                mac_entry_t *expired = *entry_ptr;
                *entry_ptr = expired->next;
                free(expired);
            } else {
                entry_ptr = &(*entry_ptr)->next;
            }
        }
    }
}

void handle_frame(switch_t *sw, const unsigned char *frame, size_t frame_len, const char *in_intf) {
    struct ether_header *eth_hdr = (struct ether_header *)frame;

    // Learn source MAC address
    add_mac_entry(sw, eth_hdr->ether_shost, in_intf);

    // Check destination MAC address
    if (memcmp(eth_hdr->ether_dhost, "\xff\xff\xff\xff\xff\xff", 6) == 0) {
        // Broadcast: Forward to all interfaces except incoming
        printf("Broadcast frame received on %s\n", in_intf);
        // Forward logic here...
    } else {
        mac_entry_t *dest_entry = find_mac_entry(sw, eth_hdr->ether_dhost);
        if (dest_entry) {
            // Known destination: Forward to specific interface
            printf("Forwarding frame to %s\n", dest_entry->interface);
            // Forward logic here...
        } else {
            // Unknown destination: Flood to all interfaces except incoming
            printf("Unknown destination: Flooding frame\n");
            // Flood logic here...
        }
    }
}

int main() {
    switch_t sw = {0};

    // Simulate receiving frames
    unsigned char frame[1518];
    size_t frame_len;

    while (1) {
        // Simulate receiving a frame on an interface
        char in_intf[] = "eth0";
        memset(frame, 0, sizeof(frame)); // Replace with actual frame data

        handle_frame(&sw, frame, frame_len, in_intf);

        // Periodically clean up expired MAC table entries
        remove_expired_entries(&sw);

        sleep(1); // Simulate time delay between frames
    }

    return 0;
}
