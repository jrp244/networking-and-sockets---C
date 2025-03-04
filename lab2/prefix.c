#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdint.h>

// Define constants for IPv4 and IPv6
#define IPV4 1
#define IPV6 2

// Helper function to convert an IP address string to an integer
uint32_t ip_str_to_int(const char *address) {
    struct in_addr addr;
    if (inet_pton(AF_INET, address, &addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", address);
        exit(EXIT_FAILURE);
    }
    return ntohl(addr.s_addr);
}

// Helper function to convert an integer to an IP address string
void ip_int_to_str(uint32_t address, char *buffer, size_t buffer_size) {
    struct in_addr addr;
    addr.s_addr = htonl(address);
    if (inet_ntop(AF_INET, &addr, buffer, buffer_size) == NULL) {
        perror("inet_ntop failed");
        exit(EXIT_FAILURE);
    }
}

// Helper function to calculate the prefix mask
uint32_t ip_prefix_mask(int prefix_len) {
    return (prefix_len == 0) ? 0 : (~0U << (32 - prefix_len));
}

// Helper function to calculate the network prefix
uint32_t ip_prefix(uint32_t address, int prefix_len) {
    uint32_t mask = ip_prefix_mask(prefix_len);
    return address & mask;
}

// Check if an IP address belongs to a given prefix
int ip_in_prefix(const char *ip_address, const char *prefix_str) {
    char prefix_ip[INET_ADDRSTRLEN];
    int prefix_len;

    // Split the prefix into IP and length
    sscanf(prefix_str, "%[^/]/%d", prefix_ip, &prefix_len);

    uint32_t ip = ip_str_to_int(ip_address);
    uint32_t prefix = ip_str_to_int(prefix_ip);
    uint32_t mask = ip_prefix_mask(prefix_len);

    return (ip & mask) == (prefix & mask);
}

// Test the functionality
void test_prefix() {
    printf("Testing Prefix.__contains__()\n");

    printf("'10.20.0.1' in '10.20.0.0/23': %s\n",
           ip_in_prefix("10.20.0.1", "10.20.0.0/23") ? "True" : "False");
    printf("'10.20.1.0' in '10.20.0.0/23': %s\n",
           ip_in_prefix("10.20.1.0", "10.20.0.0/23") ? "True" : "False");
    printf("'10.20.2.0' in '10.20.0.0/23': %s\n",
           ip_in_prefix("10.20.2.0", "10.20.0.0/23") ? "True" : "False");

    printf("'10.20.0.1' in '10.20.0.0/24': %s\n",
           ip_in_prefix("10.20.0.1", "10.20.0.0/24") ? "True" : "False");
    printf("'10.20.1.0' in '10.20.0.0/24': %s\n",
           ip_in_prefix("10.20.1.0", "10.20.0.0/24") ? "True" : "False");

    printf("'10.20.0.1' in '10.20.0.0/25': %s\n",
           ip_in_prefix("10.20.0.1", "10.20.0.0/25") ? "True" : "False");
    printf("'10.20.0.127' in '10.20.0.0/25': %s\n",
           ip_in_prefix("10.20.0.127", "10.20
