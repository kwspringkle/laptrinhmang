#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

// Định nghĩa cho Windows
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

// Implementation riêng cho inet_ntop trên Windows
const char* my_inet_ntop(int af, const void* src, char* dst, size_t size) {
    if (af == AF_INET) {
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_addr = *(struct in_addr*)src;
        DWORD len = size;
        if (WSAAddressToStringA((struct sockaddr*)&sa, sizeof(sa), NULL, dst, &len) == 0) {
            return dst;
        }
    } else if (af == AF_INET6) {
        struct sockaddr_in6 sa;
        sa.sin6_family = AF_INET6;
        sa.sin6_addr = *(struct in6_addr*)src;
        DWORD len = size;
        if (WSAAddressToStringA((struct sockaddr*)&sa, sizeof(sa), NULL, dst, &len) == 0) {
            return dst;
        }
    }
    return NULL;
}

// Implementation riêng cho inet_pton trên Windows
int my_inet_pton(int af, const char* src, void* dst) {
    if (af == AF_INET) {
        struct sockaddr_in sa;
        int len = sizeof(sa);
        if (WSAStringToAddressA((LPSTR)src, af, NULL, (struct sockaddr*)&sa, &len) == 0) {
            *(struct in_addr*)dst = sa.sin_addr;
            return 1;
        }
    } else if (af == AF_INET6) {
        struct sockaddr_in6 sa;
        int len = sizeof(sa);
        if (WSAStringToAddressA((LPSTR)src, af, NULL, (struct sockaddr*)&sa, &len) == 0) {
            *(struct in6_addr*)dst = sa.sin6_addr;
            return 1;
        }
    }
    return 0;
}

#define inet_ntop my_inet_ntop
#define inet_pton my_inet_pton

// Định nghĩa cho log file
#define LOG_FILE "resolver_log.txt"
#define MAX_LINE_LENGTH 1024
#define MAX_TOKENS 10

// Kiểm tra IPv4 hợp lệ
int is_valid_ipv4(const char *ip){
    if (inet_addr(ip) == INADDR_NONE) {
        return 0;
    }
    
    int dots = 0;
    int len = strlen(ip);
    for(int i = 0; i < len; i++) {
        if(ip[i] == '.') dots++;
        else if(ip[i] < '0' || ip[i] > '9') return 0;
    }

    return (dots == 3);
}

// Kiểm tra IPv6 hợp lệ sử dụng Windows API
int is_valid_ipv6(const char *ip) {
    struct sockaddr_in6 sa;
    return (WSAStringToAddressA((LPSTR)ip, AF_INET6, NULL, (struct sockaddr*)&sa, &(int){sizeof(sa)}) == 0);
}

// Kiểm tra có phải định dạng IP không
int looks_like_ip(const char *str) {
    int len = strlen(str);
    int dots = 0, colons = 0;
    int has_digit = 0;
    
    for(int i = 0; i < len; i++) {
        if(str[i] == '.') dots++;
        else if(str[i] == ':') colons++;
        else if(str[i] >= '0' && str[i] <= '9') has_digit = 1;
        else if((str[i] >= 'a' && str[i] <= 'f') || (str[i] >= 'A' && str[i] <= 'F')) {
            // Hex chars for IPv6
            continue;
        }
        else return 0;
    }
    
    return (dots > 0 || colons > 0) && has_digit;
}

// Kiểm tra IP đặc biệt
int is_special_ip(const char *ip) {
    struct in_addr addr;
    if (!is_valid_ipv4(ip)) return 0;
    
    addr.s_addr = inet_addr(ip);
    unsigned long ip_val = ntohl(addr.s_addr);
    
    // Loopback (127.0.0.0/8)
    if ((ip_val & 0xFF000000) == 0x7F000000) return 1;
    
    // Private addresses
    // 10.0.0.0/8
    if ((ip_val & 0xFF000000) == 0x0A000000) return 1;
    // 172.16.0.0/12
    if ((ip_val & 0xFFF00000) == 0xAC100000) return 1;
    // 192.168.0.0/16
    if ((ip_val & 0xFFFF0000) == 0xC0A80000) return 1;
    
    // Link-local (169.254.0.0/16)
    if ((ip_val & 0xFFFF0000) == 0xA9FE0000) return 1;
    
    // Multicast (224.0.0.0/4)
    if ((ip_val & 0xF0000000) == 0xE0000000) return 1;
    
    return 0;
}

// Ghi log
void write_log(const char *query, const char *result) {
    FILE *logfile = fopen(LOG_FILE, "a");
    if (logfile) {
        time_t now = time(NULL);
        char *timestr = ctime(&now);
        timestr[strlen(timestr)-1] = '\0'; // Remove newline
        fprintf(logfile, "[%s] Query: %s | Result: %s\n", timestr, query, result);
        fclose(logfile);
    }
}

//Phân giải IP sang tên miền (IPv4/IPv6)
void resolve_ip_to_domain(const char *ip){
    clock_t start = clock();
    char result_buffer[1024] = "";
    
    if (is_valid_ipv4(ip)) {
        if (is_special_ip(ip)) {
            printf("Warning: special IP address — may not have DNS record\n");
            strcat(result_buffer, "Warning: special IP address; ");
        }
    }
    
    // Sử dụng getnameinfo cho cả IPv4 và IPv6
    struct sockaddr_storage addr;
    socklen_t addr_len;
    
    if (is_valid_ipv4(ip)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in*)&addr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = 0;
        if (inet_pton(AF_INET, ip, &addr4->sin_addr) != 1) {
            addr4->sin_addr.s_addr = inet_addr(ip);
        }
        addr_len = sizeof(struct sockaddr_in);
    } else if (is_valid_ipv6(ip)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6*)&addr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = 0;
        addr6->sin6_flowinfo = 0;
        addr6->sin6_scope_id = 0;
        
        // Sử dụng WSAStringToAddress cho IPv6
        int addr_size = sizeof(struct sockaddr_in6);
        if (WSAStringToAddressA((LPSTR)ip, AF_INET6, NULL, (struct sockaddr*)addr6, &addr_size) != 0) {
            printf("Invalid IPv6 address format\n");
            return;
        }
        addr_len = sizeof(struct sockaddr_in6);
    } else {
        printf("Invalid address\n");
        strcat(result_buffer, "Invalid address");
        write_log(ip, result_buffer);
        return;
    }
    
    char hostname[NI_MAXHOST];
    int result = getnameinfo((struct sockaddr*)&addr, addr_len, 
                           hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD);
    
    if (result != 0) {
        printf("Not found information\n");
        strcat(result_buffer, "Not found information");
    } else {
        printf("Official name: %s\n", hostname);
        strcat(result_buffer, hostname);
        
        // Thử tìm aliases bằng cách resolve ngược lại
        struct addrinfo hints, *res, *ptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        printf("Alias name:\n");
        if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
            int alias_found = 0;
            for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
                char ip_str[INET6_ADDRSTRLEN];
                if (ptr->ai_family == AF_INET) {
                    struct sockaddr_in *sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
                    inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip_str, INET_ADDRSTRLEN);
                } else if (ptr->ai_family == AF_INET6) {
                    struct sockaddr_in6 *sockaddr_ipv6 = (struct sockaddr_in6*)ptr->ai_addr;
                    inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
                }
                
                if (strcmp(ip_str, ip) != 0) {
                    char alias_hostname[NI_MAXHOST];
                    if (getnameinfo(ptr->ai_addr, ptr->ai_addrlen, 
                                  alias_hostname, NI_MAXHOST, NULL, 0, 0) == 0) {
                        if (strcmp(alias_hostname, hostname) != 0) {
                            printf("%s\n", alias_hostname);
                            alias_found = 1;
                        }
                    }
                }
            }
            if (!alias_found) {
                printf("None\n");
            }
            freeaddrinfo(res);
        } else {
            printf("None\n");
        }
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Query time: %.3f seconds\n", time_taken);
    
    write_log(ip, result_buffer);
}

//Phân giải tên miền sang IP (IPv4/IPv6)
void resolve_domain_to_ip(const char *domain){
    clock_t start = clock();
    char result_buffer[1024] = "";
    
    struct addrinfo hints, *res, *ptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // IPv4 và IPv6
    hints.ai_socktype = SOCK_STREAM;
    
    int result = getaddrinfo(domain, NULL, &hints, &res);
    if (result != 0) {
        printf("Not found information\n");
        strcat(result_buffer, "Not found information");
    } else {
        printf("Canonical name (CNAME): %s\n", res->ai_canonname ? res->ai_canonname : domain);
        
        int ipv4_count = 0, ipv6_count = 0;
        char first_ip[INET6_ADDRSTRLEN] = "";
        
        // Đếm và hiển thị địa chỉ IPv4
        printf("IPv4 addresses:\n");
        for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
            if (ptr->ai_family == AF_INET) {
                struct sockaddr_in *sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip_str, INET_ADDRSTRLEN);
                
                if (ipv4_count == 0) {
                    printf("Official IP: %s\n", ip_str);
                    strcpy(first_ip, ip_str);
                    strcpy(result_buffer, ip_str);
                } else {
                    if (ipv4_count == 1) {
                        printf("Alias IP:\n");
                    }
                    printf("%s\n", ip_str);
                }
                ipv4_count++;
            }
        }
        
        if (ipv4_count == 0) {
            printf("No IPv4 addresses found\n");
        } else if (ipv4_count == 1) {
            printf("Alias IP:\nNone\n");
        }
        
        // Hiển thị địa chỉ IPv6
        printf("IPv6 addresses:\n");
        for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
            if (ptr->ai_family == AF_INET6) {
                struct sockaddr_in6 *sockaddr_ipv6 = (struct sockaddr_in6*)ptr->ai_addr;
                char ip_str[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
                
                if (ipv6_count == 0) {
                    printf("Official IPv6: %s\n", ip_str);
                    if (strlen(first_ip) == 0) {
                        strcpy(result_buffer, ip_str);
                    }
                } else {
                    if (ipv6_count == 1) {
                        printf("Alias IPv6:\n");
                    }
                    printf("%s\n", ip_str);
                }
                ipv6_count++;
            }
        }
        
        if (ipv6_count == 0) {
            printf("No IPv6 addresses found\n");
        } else if (ipv6_count == 1) {
            printf("Alias IPv6:\nNone\n");
        }
        
        freeaddrinfo(res);
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Query time: %.3f seconds\n", time_taken);
    
    write_log(domain, result_buffer);
}

// Xử lý một query đơn
void process_single_query(const char *input) {
    printf("\n--- Processing: %s ---\n", input);
    
    if (is_valid_ipv4(input) || is_valid_ipv6(input)) {
        resolve_ip_to_domain(input);
    } else if (looks_like_ip(input)) {
        printf("Invalid address\n");
        write_log(input, "Invalid address");
    } else {
        resolve_domain_to_ip(input);
    }
}

// Xử lý nhiều queries trên cùng một dòng
void process_multiple_queries(const char *line) {
    char *line_copy = malloc(strlen(line) + 1);
    strcpy(line_copy, line);
    
    char *tokens[MAX_TOKENS];
    int token_count = 0;
    
    char *token = strtok(line_copy, " \t\n");
    while (token != NULL && token_count < MAX_TOKENS) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    for (int i = 0; i < token_count; i++) {
        process_single_query(tokens[i]);
    }
    
    free(line_copy);
}

// Xử lý file batch
void process_batch_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }
    
    char line[MAX_LINE_LENGTH];
    int line_num = 1;
    
    printf("Processing batch file: %s\n", filename);
    printf("========================================\n");
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) > 0) {
            printf("\nLine %d: %s\n", line_num, line);
            process_multiple_queries(line);
            printf("----------------------------------------\n");
        }
        line_num++;
    }
    
    fclose(file);
}


int main(int argc, char *argv[]){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Batch mode: ./resolver filename.txt
    if (argc == 2) {
        FILE *test_file = fopen(argv[1], "r");
        if (test_file) {
            fclose(test_file);
            process_batch_file(argv[1]);
            WSACleanup();
            return 0;
        }
        // If not a file, treat as single query (backward compatibility)
        process_single_query(argv[1]);
        WSACleanup();
        return 0;
    }
    
    // Interactive mode
    printf("DNS Resolver - Interactive Mode\n");
    printf("===============================\n");
    printf("Enter IP addresses or domain names (empty line to quit)\n");
    printf("You can enter multiple addresses/domains on one line separated by spaces\n");
    printf("All queries are logged to %s\n\n", LOG_FILE);
    
    char input[MAX_LINE_LENGTH];
    
    while (1) {
        printf("Query> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = '\0';
        
        // Empty line to quit
        if (strlen(input) == 0) {
            printf("Goodbye!\n");
            break;
        }
        
        process_multiple_queries(input);
        printf("\n");
    }

    WSACleanup();
    return 0;
}