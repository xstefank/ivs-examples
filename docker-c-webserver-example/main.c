#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <signal.h>
#include <uuid/uuid.h>

#define HOST "ip-api.com"
#define WEB_PORT 8080
#define API_PORT 80
#define BUFFER_LENGTH 4096

int server_sockfd, client_sockfd;
char *uuid;

char *get_response() {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *hostinfo;
    char request[BUFFER_LENGTH];
    char *response = (char*) malloc(sizeof(char) * BUFFER_LENGTH);

    // Get host information
    hostinfo = gethostbyname(HOST);
    if (!hostinfo) {
        fprintf(stderr, "Error: Cannot resolve hostname\n");
        exit(1);
    }

    // Create socket 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: Cannot create socket\n");
        exit(1); 
    }

    // Set up server address information
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(API_PORT);
    memcpy(&server_addr.sin_addr.s_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Error: Cannot connect to server\n");
        exit(1); 
    }

    // Construct HTTP GET request
    sprintf(request, "GET /json HTTP/1.0\r\nHost: %s\r\n\r\n", HOST);

    // Send the request
    send(sockfd, request, strlen(request), 0);

    // Receive the response
    recv(sockfd, response, BUFFER_LENGTH, 0);
    printf("Response:\n%s\n", response); 

    // Close the socket
    close(sockfd);
    return response; 
}

char *get_country_from_response(char* response) {
    // Very basic JSON parsing
    char *country = strstr(response, "\"country\":\"") + 11;
    // char *country = strstr(response, "\"countryCode\":\"") + 15;
    char *end = strchr(country, '"');  // Find end of country value
    if(end) {
        *end = '\0'; // Terminate the string to isolate the country
    }

    return country;
}

char *get_path_from_request(char* request) {
    char *path =  strstr(request, "GET ") + 4;

    char *end = strchr(path, ' ');  // Find end of the path
    if(end) {
        *end = '\0'; // Terminate the string
    }

    return path;
}

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("Exiting...\n");

        close(server_sockfd);
        close(client_sockfd);
        exit(0);
    }
}

int main() {
    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid = malloc(37);
    uuid_unparse(binuuid, uuid);

    struct sockaddr_in server_addr, client_addr; 
    char client_request[BUFFER_LENGTH];
    socklen_t client_addr_len;

    signal(SIGINT, sig_handler);

    // Create server socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface
    server_addr.sin_port = htons(WEB_PORT);

    fprintf(stderr, "Starting to listen for requests on port %d...\n", WEB_PORT);

    // Bind socket
    if (bind(server_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0 ) {
        perror("bind failed");
        exit(1);
    }

    // Listen for connections
    if (listen(server_sockfd, 5) < 0) {
        perror("listen failed");
        exit(1);
    }

    while (1) {
        client_addr_len = sizeof(client_addr);
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addr, &client_addr_len);

        recv(client_sockfd, client_request, sizeof(client_request), 0);

        char *path = get_path_from_request(client_request);
        if (strcmp("/", path) == 0) {
            fprintf(stderr, "Received request, calling API server for response...\n");
            char *api_response = get_response();
            fprintf(stderr, "Response from API server:\n----------------\n%s\n----------------\n", api_response);

            // Construct simple HTTP response
            char response[BUFFER_LENGTH];
            sprintf(response, "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n%s\n", api_response);

            send(client_sockfd, response, strlen(response), 0);
            close(client_sockfd);
        } else if (strcmp("/whoami", path) == 0) {
            // Construct simple HTTP response
            char response[BUFFER_LENGTH];
            sprintf(response, "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\n%s\n", uuid);

            send(client_sockfd, response, strlen(response), 0);
            close(client_sockfd);
        } else {
            char response[BUFFER_LENGTH];
            sprintf(response, "HTTP/1.0 404 Not found\r\nContent-Type: text/plain\r\n\r\n%s\n", "Not found");

            send(client_sockfd, response, strlen(response), 0);
            close(client_sockfd);
        }


    }

    close(server_sockfd);
    return 0;
}
