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
#define MAX_CLIENTS 200 // Maximum number of clients
#define BUFFER_SIZE 256
#define LISTEN_BACKLOG 5
#define SLEEP_DURATION 2

int client_sockets[MAX_CLIENTS]; // Array to store the client sockets
int num_clients = 0;             // Counter for the number of clients
int burgers = 0;
pthread_mutex_t lock;
int total_burgers_made = 0;
int max_burgers = 0;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void *chef(void *arg)
{
    int id = *((int *)arg);
    int burgers_made = 0;

    while (1)
    {
        int time = rand() % 3 + 2;
        sleep(rand() % 3 + 2);
        pthread_mutex_lock(&lock);
        if (total_burgers_made < max_burgers)
        {
            burgers++;
            burgers_made++;
            total_burgers_made++;
            printf("\nChef %d made a burger in %d seconds. Burgers Available: %d, Burgers made: %d\n", id, time, burgers, total_burgers_made);
        }
        pthread_mutex_unlock(&lock);
        if (total_burgers_made >= max_burgers)
        {
            break;
        }
    }

    free(arg);
    return NULL;
}

void *client_handler(void *arg)
{
    int sock = *((int *)arg);

    // Read the number of burgers wanted from the client
    char buffer[BUFFER_SIZE];
    read(sock, buffer, 255);
    int burgers_wanted = atoi(buffer);

    char message[BUFFER_SIZE];
    int burgers_served = 0;
    while (1)
    {
        sleep(rand() % 5 + 1);
        pthread_mutex_lock(&lock);
        if (burgers > 0 && burgers_served < burgers_wanted)
        {
            burgers--;
            burgers_served++;
            sprintf(message, "You got a burger! Burgers left: %d\n", burgers);
            printf("Serving a burger to client %d. Burgers left: %d. Client %d still wants %d burgers.\n", sock, burgers, sock, burgers_wanted - burgers_served);
            write(sock, message, strlen(message));
        }
        else if (burgers == 0 && burgers_served == max_burgers)
        {
            // Break the loop if the restaurant has served all the burgers
            break;
        }
        pthread_mutex_unlock(&lock);
        if (burgers_served >= burgers_wanted)
        {
            printf("Client %d no longer wants burgers.\n", sock);
            break;
        }
    }

    close(sock);
    return NULL;
}

void *accept_clients(void *arg)
{
    int sockfd = *((int *)arg);
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    while (1)
    {
        int *newsockfd = malloc(sizeof(int));
        *newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (*newsockfd < 0)
            error("ERROR on accept");
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, client_handler, (void *)newsockfd);
    }
}

void validate_args(int argc, char *argv[])
{
    int default_max_burgers = 25;
    int default_num_chefs = 2;

    if (argc < 3)
    {
        fprintf(stderr, "Insufficient arguments provided. Using default values.\n");
        argv[1] = (char *)malloc(4);
        sprintf(argv[1], "%d", default_max_burgers);
        argv[2] = (char *)malloc(3);
        sprintf(argv[2], "%d", default_num_chefs);
    }

    int max_burgers = atoi(argv[1]);
    if (max_burgers <= 0 || max_burgers > 1000)
    {
        fprintf(stderr, "Invalid number of burgers. Using default value %d.\n", default_max_burgers);
        sprintf(argv[1], "%d", default_max_burgers);
    }

    int num_chefs = atoi(argv[2]);
    if (num_chefs <= 0 || num_chefs > 50)
    {
        fprintf(stderr, "Invalid number of chefs. Using default value %d.\n", default_num_chefs);
        sprintf(argv[2], "%d", default_num_chefs);
    }
}

void handle_error(const char *msg)
{
    perror(msg);
    exit(0);
}

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        handle_error("ERROR opening socket");

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        handle_error("setsockopt");
    }

    return sockfd;
}

int main(int argc, char *argv[])
{
    validate_args(argc, argv);

    max_burgers = atoi(argv[1]);
    int num_chefs = atoi(argv[2]);

    srand(time(NULL));
    pthread_mutex_init(&lock, NULL);

    pthread_t chef_thread[num_chefs];
    for (long i = 0; i < num_chefs; i++)
    {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&chef_thread[i], NULL, chef, (void *)id);
    }

    int sockfd = create_socket();
    int portno = PORT;

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        handle_error("ERROR on binding");

    listen(sockfd, LISTEN_BACKLOG);

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_clients, (void *)&sockfd);

    while (1)
    {
        // Check if all burgers have been produced and served
        if (total_burgers_made >= max_burgers && burgers == 0)
        {
            break;
        }
    }

    // Wait for all chef threads to finish
    for (long i = 0; i < num_chefs; i++)
    {
        pthread_join(chef_thread[i], NULL);
    }

    printf("The restaurant is closing.\n");

    // Add a delay before closing the socket
    sleep(SLEEP_DURATION);

    close(sockfd);
    return 0;
}