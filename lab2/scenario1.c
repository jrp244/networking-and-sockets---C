#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/icmp.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <unistd.h>

void handle_frame(const unsigned char *frame) {
    struct ether_header *eth = (struct ether_header *)frame;

    // Check if it's an IPv4 packet
    if (ntohs(eth->ether_type) == ETHERTYPE_IP) {
        struct iphdr *ip = (struct iphdr *)(frame + sizeof(struct ether_header));

        // Check if it's an ICMP packet
        if (ip->protocol == IPPROTO_ICMP) {
            struct icmphdr *icmp = (struct icmphdr *)((unsigned char *)ip + ip->ihl * 4);

            // Check ICMP type
            if (icmp->type == ICMP_ECHO || icmp->type == ICMP_ECHOREPLY) {
                printf("Received ICMP packet from %s\n", inet_ntoa(*(struct in_addr *)&ip->saddr));
            }
        }
    }
}
void send_icmp_echo(const char *dst_ip) {
    printf("Sending ICMP Echo Request to %s\n", dst_ip);
    // Implement raw socket code here for sending ICMP packets
}
void drop_link(const char *intf) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "iptables -A OUTPUT -o %s -j DROP", intf);
    system(cmd);
    printf("Dropped link on interface %s\n", intf);
}

void schedule_items() {
    sleep(6); // Simulate START_TIME delay
    printf("START\n");
    send_icmp_echo("r5");
    sleep(3); // Simulate additional delay
    printf("STOP\n");
}

// Function prototypes
void handle_frame(const unsigned char *frame);
void send_icmp_echo(const char *dst_ip);
void drop_link(const char *intf);
void schedule_items();

int main() {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    printf("Hostname: %s\n", hostname);

    if (strcmp(hostname, "r1") == 0) {
        printf("SimHost1 selected.\n");
        schedule_items();
    } else if (strcmp(hostname, "r2") == 0) {
        printf("SimHost2 selected.\n");
        schedule_items();
    } else {
        printf("Default SimHost selected.\n");
        schedule_items();
    }

    return 0;
}
