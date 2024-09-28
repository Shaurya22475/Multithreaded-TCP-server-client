#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(char *msg) {
    perror(msg);
    exit(1);
}

void *handle_client(void *newsockfd_ptr) {
    int newsockfd = *(int *)newsockfd_ptr;
    free(newsockfd_ptr);

    char buffer[256]; // Buffer to read characters from connection
    bzero(buffer, 256);

    int n = read(newsockfd, buffer, 255);
    if (n < 0) {
        error("ERROR reading from socket");
    }

    printf("Here is the message: %s\n", buffer);

    n = write(newsockfd, "I got your message", 18);
    if (n < 0) {
        error("ERROR writing to socket");
    }

    close(newsockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "Error, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        int *newsockfd = malloc(sizeof(int)); 
        if (newsockfd == NULL) {
            error("ERROR allocating memory for client socket");
        }

        *newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        fprintf(stdout,"Connected to %d:\n", *newsockfd);
        fflush(stdout);

        if (*newsockfd < 0) {
            error("ERROR on accept");
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *) newsockfd) != 0) {
            error("ERROR creating thread");
        }

        pthread_detach(client_thread); 
    }

    close(sockfd); 
    return 0;
}
