#include "commands/iman.h"
#include "utils/error.h"
#include "utils/colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Constants moved from the old header file to the .c file
#define HOSTNAME "man.he.net"
#define SERVER_IP "65.19.140.5" // Note: Using a hardcoded IP is not robust, but preserved as requested.
#define SERVER_PORT 80
#define BUFFER_SIZE 900000 // For heap allocation

void printFormatted(char* start_ptr, char* end_ptr) {
    char* curr = start_ptr;
    char ans[900000] = {};
    while(curr && curr < end_ptr){
        char* new = strstr(curr, "<STRONG>");
        if(new > end_ptr)
            new = end_ptr;
        strcpy(ans, "");
        strncpy(ans, curr, new - curr);
        ans[new - curr] = '\0';
        printf("%s", ans);

        char name[1000] = {};
        char* name_start = strstr(new, "\">") + 2;
        char* name_end = strstr(new, "</A>");
        if(name_start - 2 && name_end && name_start < end_ptr) {
            strncpy(name, name_start, name_end - name_start);
            name[name_end - name_start] = '\0';
            printf(_BLUE_"%s"_RESET_ " ", name);
        }

        curr = strstr(new, "</STRONG>");
        curr += 10;
    }
}


// Your original iMan function, renamed and adapted
void iman_execute(const char* command_name) {
    char address[100] = {0};
    snprintf(address, sizeof(address), "/?topic=%s&section=all", command_name);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        print_shell_error("iman: Socket creation failed");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        print_shell_error("iman: Connection failed");
        close(sockfd);
        return;
    }

    char request[5000] = {0};
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", address, HOSTNAME);
    if (send(sockfd, request, strlen(request), 0) == -1) {
        print_shell_error("iman: Request sending failed");
        close(sockfd);
        return;
    }

    // CRITICAL FIX: Allocate buffer on the heap, not the stack
    char* buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        print_shell_perror("iman: Failed to allocate memory");
        close(sockfd);
        return;
    }

    // Using recv in a loop is better, but this preserves the original MSG_WAITALL logic
    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, MSG_WAITALL);
    close(sockfd); // Close socket as soon as data is received

    if (bytes_received <= 0) {
        print_shell_error("iman: Response receiving failed or connection closed");
        free(buffer);
        return;
    }
    buffer[bytes_received] = '\0';

    if(strstr(buffer, "Search Again") != NULL){
        printf(_RED_"ERROR: "_RESET_"iman: Command not found\n");
        free(buffer);
        return;
    }
    // print_shell_error(buffer);
    char* start_ptr = strstr(buffer, "NAME\n");
    if (!start_ptr) {
        print_shell_error("iman: Could not find 'NAME' section in man page.");
        free(buffer);
        return;
    }

    
    char* end_ptr = NULL;
    end_ptr = strstr(buffer, "</pre>");
    if(!end_ptr) end_ptr = strstr(buffer, "Linux                             2");
    if(!end_ptr) end_ptr = strstr(buffer, "Linux                             1");
    if(!end_ptr) end_ptr = strstr(buffer, "GNU                               2");
    if(!end_ptr) end_ptr = strstr(buffer, "GNU                               1");
    if(!end_ptr) end_ptr = strstr(buffer, "</PRE");

    if(!end_ptr){
        print_shell_error("iman: Could not find end of man page content.");
        free(buffer);
        return;
    }

    // Call your original formatting function
    printFormatted(start_ptr, end_ptr);
    printf("\n");

    free(buffer); // Free the heap-allocated memory
}