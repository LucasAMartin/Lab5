#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 54321
#define MAX_BURGERS 10

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    int burgers_eaten = 0;
    int burgers_wanted = (argc > 3) ? atoi(argv[3]) : 10; // Use the command line argument if provided, otherwise default to 10
    // Send the number of burgers wanted to the server
    char buffer[256];
    sprintf(buffer, "%d", burgers_wanted);
    write(sockfd, buffer, strlen(buffer));
    
    while (burgers_eaten < burgers_wanted) {
        bzero(buffer,256);
        n = read(sockfd,buffer,255);
        if (n < 0) 
             error("ERROR reading from socket");
        printf("%s\n",buffer);
        if(strstr(buffer, "You got a burger!") != NULL) {
            burgers_eaten++;
            printf("I have eaten %d burgers. I still want %d burgers.\n", burgers_eaten, burgers_wanted - burgers_eaten);
        } else if (strstr(buffer, "The restaurant is closing.") != NULL) {
            printf("The restaurant is closing. Exiting...\n");
            break;
        }
    }

    printf("I no longer want burgers.\n");

    close(sockfd);
    return 0;
}

