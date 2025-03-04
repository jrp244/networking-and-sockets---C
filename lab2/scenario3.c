#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/icmp.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>

#define START_TIME 12

// Function prototypes
void handle_frame(const unsigned char *frame, const char *intf);
void send_icmp_echo(const char *dst);
void drop_link(const char *intf);
void schedule_simhost2();
void schedule_simhost6();
void schedule_simhost7();
void schedule_simhost8();
void schedule_simhost9();
void schedule_default();
void *event_loop(void *arg);

// Global variables for event scheduling
struct timeval start_time;

// Helper function to get the current time in seconds
double get_time_in_seconds() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return (current_time.tv_sec - start_time.tv_sec) +
           (current_time.tv_usec - start_time.tv_usec) / 1e6;
}

// Handle an Ethernet frame (simplified)
void handle_frame(const unsigned char *frame, const char *intf) {
    struct ether_header *eth = (struct ether_header *)frame;

    // Check if it's an IPv4 packet
    if (ntohs(eth->ether_type) == ETHERTYPE_IP) {
        struct iphdr *ip = (struct iphdr *)(frame + sizeof(struct ether_header));

        // Check if it's an ICMP packet
        if (ip->protocol == IPPROTO_ICMP) {
            struct icmphdr *icmp = (struct icmphdr *)((unsigned char *)ip + ip->ihl * 4);

            // Check ICMP type
            if (icmp->type == ICMP_ECHO || icmp->type == ICMP_ECHOREPLY) {
                char src_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &ip->saddr, src_ip, sizeof(src_ip));
                printf("Received ICMP packet from %s on interface %s.\n", src_ip, intf);
            }
        }
    }
}

// Send an ICMP Echo Request using the `ping` command
void send_icmp_echo(const char *dst) {
    printf("Sending ICMP Echo Request to %s\n", dst);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ping -W 1 -c 1 %s", dst);
    system(cmd);
}

// Drop a link using a simulated command
void drop_link(const char *intf) {
    printf("Dropping link %s\n", intf);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "iptables -A OUTPUT -o %s -j DROP", intf);
    system(cmd);
}

// Schedule events for SimHost2
void schedule_simhost2() {
    sleep(START_TIME);
    printf("START\n");
    sleep(8); // Simulate delay for dropping link
    drop_link("r2-r8");
    sleep(12); // Simulate delay for stopping
    printf("STOP\n");
}

// Schedule events for SimHost6
void schedule_simhost6() {
    sleep(START_TIME + 5);
    send_icmp_echo("r14");
}

// Schedule events for SimHost7
void schedule_simhost7() {
    sleep(START_TIME + 6);
    send_icmp_echo("r15");
    sleep(13); // Simulate delay
    send_icmp_echo("r15");
}

// Schedule events for SimHost8
void schedule_simhost8() {
    sleep(START_TIME + 8);
    drop_link("r8-r2");
}

// Schedule events for SimHost9
void schedule_simhost9() {
    sleep(START_TIME + 1);
    send_icmp_echo("r10");
    sleep(1); // Simulate delay
    send_icmp_echo("r11");
    sleep(1); // Simulate delay
    send_icmp_echo("r12");
    sleep(1); // Simulate delay
    send_icmp_echo("r13");
}

// Default scheduler (does nothing)
void schedule_default() {
    printf("No specific events scheduled for this host.\n");
}

// Event loop thread function (optional for asynchronous behavior)
void *event_loop(void *arg) {
    const char *hostname = (const char *)arg;

    if (strcmp(hostname, "r2") == 0) {
        schedule_simhost2();
    } else if (strcmp(hostname, "r6") == 0) {
        schedule_simhost6();
    } else if (strcmp(hostname, "r7") == 0) {
        schedule_simhost7();
    } else if (strcmp(hostname, "r8") == 0) {
        schedule_simhost8();
    } else if (strcmp(hostname, "r9") == 0) {
        schedule_simhost9();
    } else {
        schedule_default();
    }

    return NULL;
}

int main() {
    // Get the hostname of the machine
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    
    printf("Hostname: %s\n", hostname);

    // Record the start time for scheduling purposes
    gettimeofday(&start_time, NULL);

    // Create a thread to run the event loop
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, event_loop, hostname);

    // Wait for the event loop to finish
    pthread_join(thread_id, NULL);

    return 0;
}
