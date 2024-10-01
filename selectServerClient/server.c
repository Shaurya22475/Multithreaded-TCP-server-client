#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

struct Process
{
    char name[256];                     
    int pid;                            
    unsigned long long total_time;      
};

void error(const char *msg) 
{
    perror(msg);
    exit(1);
}

int get_info(char *pid, struct Process *process)
{
    char path[256];
    snprintf(path,sizeof(path),"/proc/%s/stat",pid);
    FILE *proc_stat=fopen(path,"r");
    if(proc_stat==NULL)
    {
        return 1;
    }
    fscanf(proc_stat,"%d (%[^)])",&process->pid,process->name);
    for (int i = 0; i < 12; i++)
    {
        fscanf(proc_stat,"%*s");
    }
    unsigned long long user_time, kernel_time;
    fscanf(proc_stat,"%llu %llu",&user_time, &kernel_time);
    process->total_time=user_time+kernel_time;
    fclose(proc_stat);
    return 0;
}

void calculating_top_two_process(struct Process top[2])
{
    DIR *proc=opendir("/proc");
    if(proc ==NULL)
    {
        error("Director opening has failed");
    }
    struct dirent *entry;
    memset(top,0,sizeof(struct Process)*2);
    while ((entry = readdir(proc)) != NULL) 
    {
        if (entry->d_type == DT_DIR) 
        {
            long pid = strtol(entry->d_name, NULL, 10);
            if (pid == 0) {
                continue; 
            }

            struct Process current_proc;

            if (get_info(entry->d_name, &current_proc) == 0) 
            {
                if (current_proc.total_time > top[0].total_time) 
                {
                    top[1] = top[0]; 
                    top[0] = current_proc; 
                }   
                else if (current_proc.total_time > top[1].total_time) 
                {
                    top[1] = current_proc; 
                }
            }
        }
    }
    closedir(proc); 
}

void handle_client(int sockfd) {
    char buffer[256];
    bzero(buffer, 256);

    // Send protocol info to client
    char protocol_info[] = "Send 'GET /TOP2' to retrieve the top two processes.\n";
    int n = write(sockfd, protocol_info, strlen(protocol_info));
    if (n < 0) error("ERROR writing protocol info to socket");

    n = read(sockfd, buffer, 255);
    if (n < 0) error("ERROR reading from socket");
    printf("Message from client: %s\n", buffer);

    bzero(buffer, 256);
    n = read(sockfd, buffer ,255);
    if (n < 0){
        error("ERROR reading from socket");
    }

    if (strcmp(buffer, "GET /TOP2") == 0) {
        struct Process top[2];
        calculating_top_two_process(top);

        char response[1024];
        snprintf(response, sizeof(response),
                 "Top 1: PID=%d, Name=%s, Total Time=%llu\n"
                 "Top 2: PID=%d, Name=%s, Total Time=%llu\n",
                 top[0].pid, top[0].name, top[0].total_time,
                 top[1].pid, top[1].name, top[1].total_time);

        n = write(sockfd, response, strlen(response));
        if (n < 0) error("ERROR writing to socket");
    } else {
        char error_msg[] = "Invalid request. Use 'GET /TOP2'.\n";
        n = write(sockfd, error_msg, strlen(error_msg));
        if (n < 0) error("ERROR writing to socket");
    }

    close(sockfd);
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
    if (sockfd < 0) error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    fd_set active_fd_set, read_fd_set;
    FD_ZERO(&active_fd_set);
    FD_SET(sockfd, &active_fd_set);  

    while (1) {
        read_fd_set = active_fd_set;  


        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            error("ERROR on select");
        }


        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == sockfd) {
                    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                    if (newsockfd < 0) error("ERROR on accept");

                    printf("Connected to client socket %d\n", newsockfd);
                    FD_SET(newsockfd, &active_fd_set);
                } else {
                  
                    handle_client(i);
                    FD_CLR(i, &active_fd_set);  
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
