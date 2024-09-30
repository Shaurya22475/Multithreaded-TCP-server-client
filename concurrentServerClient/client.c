#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>

#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

void error(char *msg){
    perror(msg);
    exit(1);
}

void *client_thread(void *arg){
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    char **thread_args = (char **)arg;
    char *hostname = thread_args[0];
    int server_port = atoi(thread_args[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        error("ERROR opening socket");
    }

    server = gethostbyname(hostname);

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host \n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(server_port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR connecting");
    }

    printf("sending message from client thread %ld\n", pthread_self());
    bzero(buffer, 256);
    snprintf(buffer, 255, "Hello from client thread %ld", pthread_self());
    n = write(sockfd, buffer , strlen(buffer));
    if (n<0){
        error("ERROR writing to socket");
    }

    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0){
        error("ERROR reading from socket");
    }

    printf("Server reply to thread %ld:\n%s\n", pthread_self(), buffer);

    bzero(buffer, 255);
    snprintf(buffer, 255, "GET /TOP2");
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0){
        error("ERROR connecting");
    }

    bzero(buffer, 255);
    n = read(sockfd, buffer, 255);
    if (n < 0){
        error("ERROR reading from socket");
    }

    printf("Server Response :: %s\n", buffer);

    close(sockfd);
    return NULL;

}

int main(int argc, char* argv[]){
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <hostname> <server_port> <number_of_connections>\n", argv[0]);
        exit(1);
    }

    char *hostname = argv[1];
    char *server_port = argv[2];
    int num_connections = atoi(argv[3]);

    pthread_t threads[num_connections];

    for (int i = 0; i < num_connections; i++) {
        char *thread_args[2];
        thread_args[0] = hostname;
        thread_args[1] = server_port;
        if (pthread_create(&threads[i], NULL, client_thread, (void *)thread_args) != 0) {
            error("ERROR creating thread");
        }
    }


    for (int i = 0; i < num_connections; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;

}
