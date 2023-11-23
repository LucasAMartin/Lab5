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

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void validate_args(int argc, char *argv[]) {
    char* default_hostname = "127.0.0.1";
    int default_portno = 54321;
    int default_burgers = 10;

    if (argc < 4) {
        fprintf(stderr, "Insufficient arguments provided. Using default values.\n");
        argv[1] = default_hostname;
        argv[2] = (char*) malloc(6);
        sprintf(argv[2], "%d", default_portno);
        argv[3] = (char*) malloc(4);
        sprintf(argv[3], "%d", default_burgers);
    }

    int portno = atoi(argv[2]);
    if (portno <= 0 || portno > 65535) {
        fprintf(stderr, "Invalid port number. Using default value %d.\n", default_portno);
        sprintf(argv[2], "%d", default_portno);
    }

    int burgers_wanted = atoi(argv[3]);
    if (burgers_wanted <= 0 || burgers_wanted > 200) {
        fprintf(stderr, "Invalid number of burgers. Using default value %d.\n", default_burgers);
        sprintf(argv[3], "%d", default_burgers);
    }
}


int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    validate_args(argc, argv);

    char* hostname = argv[1];
    portno = atoi(argv[2]);
    int burgers_wanted = atoi(argv[3]);
    int burgers_eaten = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0],
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // Send the number of burgers wanted to the server
    char buffer[256];
    sprintf(buffer, "%d", burgers_wanted);
    write(sockfd, buffer, strlen(buffer));

    while (burgers_eaten < burgers_wanted)
    {
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);
        if (n <= 0)
        {
            if (n == 0)
            {
                printf("The restaurant is closing. Exiting...\n");
            }
            else
            {
                perror("ERROR reading from socket");
            }
            close(sockfd);
            exit(0);
        }

        printf("%s\n", buffer);
        if (strstr(buffer, "You got a burger!") != NULL)
        {
            burgers_eaten++;
            printf("I have eaten %d burgers. I still want %d burgers.\n", burgers_eaten, burgers_wanted - burgers_eaten);
        }
    }
    printf("I no longer want burgers.\n");
    close(sockfd);
    return 0;
}
