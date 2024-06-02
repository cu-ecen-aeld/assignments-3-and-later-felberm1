#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <arpa/inet.h>

static int server_fd = 0;
static int client_fd = 0; 
static FILE* fd = NULL;

static void graceful_stop(int signum){
    syslog(LOG_DEBUG, "Caught signal, exiting");
    if (server_fd > 0){
        close(server_fd);
        syslog(LOG_DEBUG, "Closing server socket");
    }
    if (client_fd > 0){
        close(client_fd);
        syslog(LOG_DEBUG, "Closing client socket");
    }
    if (fd != NULL){
        fclose(fd);
        syslog(LOG_DEBUG, "Closing file");
    }
}

static void daemonize(){
    pid_t pid;

    pid = fork();

    //exit on error
    if(pid < 0){
        syslog(LOG_DEBUG, "Fork #1 failed");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_DEBUG, "Fork #1 done");

    //stop parent
    if(pid > 0){
        syslog(LOG_DEBUG, "Stop parent #1");
        exit(EXIT_SUCCESS);
    }

    //change session id exit on failure
    if(setsid() < 0){
        syslog(LOG_DEBUG, "Set SID failed");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_DEBUG, "Set SID done");

    //fork again to deamonize
    pid = fork();

    //exit on error
    if(pid < 0){
        syslog(LOG_DEBUG, "Fork #2 failed");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_DEBUG, "Fork #2 done");

    //stop parent
    if(pid > 0){
        syslog(LOG_DEBUG, "Stop parent #2");
        exit(EXIT_SUCCESS);
    }

    //add signal handlers again
    signal(SIGINT, graceful_stop);
    signal(SIGTERM, graceful_stop);
    syslog(LOG_DEBUG, "Registered daemon signal handler");
}

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv){

    setlogmask(LOG_UPTO (LOG_DEBUG));
    openlog("aesdsocket.log", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    syslog(LOG_DEBUG, "Starting aesdserver");


    signal(SIGINT, graceful_stop);
    signal(SIGTERM, graceful_stop);
    syslog(LOG_DEBUG, "Registered signal handler");

    int ret;
    struct addrinfo hints;
    struct addrinfo* servinfo;

    //socket configurations
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    
    //get socket info
    ret = getaddrinfo(NULL,"9000", &hints, &servinfo);
    if (ret < 0){
        syslog(LOG_DEBUG, "Unable to get address info");
        return -1;
    }
    //create socket fd
    server_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (ret < 0){
        syslog(LOG_DEBUG, "Unable to create server socket");
        return -1;
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); 
    //bind socket to address
    ret = bind(server_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (ret < 0){
        syslog(LOG_DEBUG, "Socket bind failed");
        return -1;
    }

    //check if program should be deamonized
    if(argc ==2){
        if(strcmp(argv[1], "-d") == 0){
            syslog(LOG_DEBUG, "Turning into a deamon");
            daemonize();
        }
    }

    //free memory
    freeaddrinfo(servinfo);

    //start listening
    ret = listen(server_fd,1);
    if (ret < 0){
        syslog(LOG_DEBUG, "Unable to listen for new connection");
        return -1;
    }

    fd = fopen("/var/tmp/aesdsocketdata","w");
    if (fd == NULL){
        syslog(LOG_DEBUG, "File cannot be opened");
        return -1;
    }
    fclose(fd);
    syslog(LOG_DEBUG, "File cleared");

    //wait for client connection
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char client_ip[INET6_ADDRSTRLEN];

    while (true) {

        syslog(LOG_DEBUG, "Waiting for new client");
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_fd < 0){
            syslog(LOG_DEBUG, "Connection accept failed");
            exit(EXIT_FAILURE);
        }
        inet_ntop(client_addr.ss_family,get_in_addr((struct sockaddr *)&client_addr),
            client_ip, sizeof client_ip);
        syslog(LOG_DEBUG, "Accepted connection from %s", client_ip);
        //setup buffers
        char in_buff[512];
        char out_buff[512];
        memset(in_buff,0,sizeof(in_buff));
        memset(out_buff,0,sizeof(out_buff));

        //receive message and dump to a file
        fd = fopen("/var/tmp/aesdsocketdata","a");
        if (fd == NULL){
            syslog(LOG_DEBUG, "File cannot be opened");
        }
        syslog(LOG_DEBUG, "File opened for appending");

        bool package_end = false;
        int byte_recv;
        while(!package_end){

            byte_recv = recv(client_fd,in_buff,sizeof(in_buff),0);
            syslog(LOG_DEBUG, "Received %d bytes: %s", byte_recv, in_buff);

            ret = fwrite(in_buff, sizeof(char), strlen(in_buff), fd);
            memset(in_buff,0,sizeof(in_buff));
            syslog(LOG_DEBUG, "Wrote %d bytes: %s", ret, in_buff);

            //test received package end aka '\n'
            if (strcmp(&in_buff[byte_recv], "\n") && byte_recv != 512){
                package_end = true;
                syslog(LOG_DEBUG, "End recive");
            }
        }

        fclose(fd);
        syslog(LOG_DEBUG, "File closed");

        fd = fopen("/var/tmp/aesdsocketdata","r");
        if (fd == NULL){
            syslog(LOG_DEBUG, "File cannot be opened");
        }
        syslog(LOG_DEBUG, "File opened for reading");

        int byte_read = 0;
        bool end_of_file = false;
        while (!end_of_file) {

            byte_read = fread(out_buff, sizeof(char), sizeof(out_buff), fd);  
            syslog(LOG_DEBUG, "Read %d bytes", byte_read);
            if(byte_read > 0){
                send(client_fd,out_buff,byte_read,0);
                syslog(LOG_DEBUG, "Sending %d bytes: %s",byte_read,out_buff);
            }else {
                end_of_file = true;
                syslog(LOG_DEBUG, "End send");
            }
            memset(out_buff,0,sizeof(out_buff));
        }
        fclose(fd);
        syslog(LOG_DEBUG, "File closed");
        close(client_fd);
        syslog(LOG_DEBUG, "Closed connection from %s", client_ip);
    }
}