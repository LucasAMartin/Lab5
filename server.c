#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#define PORT 54321
#define MAX_BURGERS 25
#define MAX_CHEFS 2

int burgers = 0;
pthread_mutex_t lock;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int total_burgers_made = 0;

void *chef(void *arg)
{
    int burgers_made = 0;
    while (1)
    {
        sleep(rand() % 3 + 2);
        pthread_mutex_lock(&lock);
        if (total_burgers_made < MAX_BURGERS)
        {
            burgers++;
            burgers_made++;
            total_burgers_made++;
            printf("Chef %ld made a burger. Total burgers in stock: %d, Total burgers made: %d\n", (long)arg, burgers, total_burgers_made);
        }
        pthread_mutex_unlock(&lock);
        if (total_burgers_made >= MAX_BURGERS)
        {
            printf("All burgers are produced. The restaurant will remain open until all burgers are served.\n");
            break;
        }
    }
    return NULL;
}

typedef struct {
    int sock;
    int burgers_wanted;
} client_info;

void *client_handler(void *arg) {
    client_info *info = (client_info *)arg;
    int sock = info->sock;
    // Read the number of burgers wanted from the client
    char buffer[256];
    read(sock, buffer, 255);
    int burgers_wanted = atoi(buffer);
    char message[256];
    int burgers_served = 0;
    while (1) {
        sleep(rand() % 5 + 1);
        pthread_mutex_lock(&lock);
        if (burgers > 0 && burgers_served < burgers_wanted) {
            burgers--;
            burgers_served++;
            sprintf(message, "You got a burger! Burgers left: %d\n", burgers);
            printf("Serving a burger to client %d. Burgers left: %d. Client %d still wants %d burgers.\n", sock, burgers, sock, burgers_wanted - burgers_served);
            write(sock, message, strlen(message));
        }
        pthread_mutex_unlock(&lock);
        if (burgers_served >= burgers_wanted) {
            printf("Client %d no longer wants burgers.\n", sock);
            break;
        }
    }
    close(sock);
    free(arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    pthread_mutex_init(&lock, NULL);

    pthread_t chef_thread[MAX_CHEFS];
    for (long i = 0; i < MAX_CHEFS; i++) {
        pthread_create(&chef_thread[i], NULL, chef, (void *)i);
    }

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = PORT;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");

        pthread_t client_thread;
        client_info *info = malloc(sizeof(client_info));
        info->sock = newsockfd;
        info->burgers_wanted = /* number of burgers this client wants */
        pthread_create(&client_thread, NULL, client_handler, (void*) info);
    }

    close(sockfd);
    return 0; 
}
