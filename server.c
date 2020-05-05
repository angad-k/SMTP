#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#define SMTP_PORT 24

int setup_server()
{
    /*server_fd is the main socket with which we'll carry out the rest of the flow.
    AF_INET means we use IPv4, SOCK_STREAM refers to TCP, 0 is the default configuration.*/
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == 0)
    {
        perror("socket failed"); 
        exit(EXIT_FAILURE);
    }

    //opt will be used in the next operation
    int opt = 1; 

    /*Here, setsockopt has five parameters, first is the socket, second is the level where we are applying the parameters.
    Third are the options to which we are applying. SO_REUSEADDR will let us bind to a socket even if the previous user of it hasn't closed it completely.
    SO_REUSE port will allow us to let multiple servers run on the same port.
    Fourth parameter is the option value. Fifth is its's size*/
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))                                              
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    }
    return server_fd;
}

struct sockaddr_in get_addr()
{
    /* sockaddr is a struct that has just two members, family and data. 
    To easily set up the address, we use the sockaddr_in struct which makes it much easier to set up the member variables.
    sockaddr_in and sockaddr can be typecasted to each other and that's exactly what we will do. :) */
    struct sockaddr_in address;
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( SMTP_PORT );
    return address;
}

void attach_server(int server_fd,struct sockaddr_in address)
{
    // Attaching the server to port 25.
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
}

void start_listening(int server_fd)
{
    //Start listening
    if (listen(server_fd, 5) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
}

int accept_connection(int server_fd, struct sockaddr_in address, int addrlen)
{
    //Accept connections
    int client_sockfd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (client_sockfd < 0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    }
    return client_sockfd;
}

void send_mail_handler(int client_sockfd)
{
    char buffer[1024];
    memset(buffer, 0, 1024);

    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_sockfd;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, clientIP, INET_ADDRSTRLEN );

    memset(buffer, 0, 1024);
    strcpy(buffer, "Your IP address : \n");
    strcat(buffer, clientIP);
    strcat(buffer, "\nPlease enter destination IP address. \n");
    if(send(client_sockfd, buffer, sizeof(buffer), 0) < 0)
    {
        printf("Error during sending \n");
    }

    memset(buffer, 0, 1024);
    recv(client_sockfd, buffer, sizeof(buffer), 0);
    char toIP[INET_ADDRSTRLEN];
    strncpy(toIP, buffer, strlen(buffer)-1);

    memset(buffer, 0, 1024);
    strcat(buffer, "Complete the email below. Send 'END_OF_EMAIL' on a lone line to end the email. Character limit is 10000.\nFrom : ");
    strcat(buffer, clientIP);
    strcat(buffer, "\nTo : ");
    strcat(buffer, toIP);
    strcat(buffer, "\nBody : \n");
    if(send(client_sockfd, buffer, sizeof(buffer), 0) < 0)
    {
        printf("Error during sending \n");
    }

    char body[10000];
    memset(body, 0, 10000);
    memset(buffer, 0, 1024);
    while(strcmp(buffer, "END_OF_EMAIL\n") != 0)
    {
        strcat(body, buffer);
        memset(buffer, 0, 1024);
        recv(client_sockfd, buffer, sizeof(buffer), 0);
        
    }
    close(client_sockfd);
    printf("%s", body);

    char emailname[100];
    memset(emailname, 0, 100);
    strcat(emailname, toIP);
    strcat(emailname, "_");
    strcat(emailname, clientIP);
    strcat(emailname, "_");
    srand(time(0));
    int random_number = rand()%1000 + 1;
    char random_string[4];
    sprintf(random_string, "%d", random_number);
    strcat(emailname, random_string);

    FILE *fp;
    fp = fopen(emailname, "w");
    fprintf(fp, "From : %s\nTo : %s\nBody : \n%s", clientIP, toIP, body);
    fclose(fp);

    fp = fopen(toIP, "a");
    fprintf(fp, "%s|", emailname);
    fclose(fp);
}

void recv_mail_handler(int client_sockfd)
{
    char buffer[1024];
    memset(buffer, 0, 1024);

    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_sockfd;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, clientIP, INET_ADDRSTRLEN );

    FILE *fp;
    fp = fopen(clientIP, "r");
    if(fp == NULL)
    {
        strcpy(buffer, "There are no unread emails. Have a nice day!\n");
        if(send(client_sockfd, buffer, sizeof(buffer), 0) < 0)
        {
            printf("Error during sending \n");
        }
        close(client_sockfd);
    }
    else
    {
        strcpy(buffer, "Seems like you have some unread emails. Sit tight while we retrieve them for you!\n");
        if(send(client_sockfd, buffer, sizeof(buffer), 0) < 0)
        {
            printf("Error during sending \n");
        }
        memset(buffer, 0, 1024);
        char emailname[100];
        memset(emailname, 0, 100);
        do
        {
            char c = fgetc(fp);
            if(c == '|')
            {
                retrieve_mail(client_sockfd, emailname);
                memset(emailname, 0, 100);
            }
            else
            {
                char cstr[2];
                sprintf(cstr, "%c", c);
                strcat(emailname, cstr);
            }
            
        } while (!feof(fp));

        strcpy(buffer, "DONE");
        if(send(client_sockfd, buffer, sizeof(buffer), 0) < 0)
        {
            printf("Error during sending \n");
        }
        close(client_sockfd);
    }
}

void retrieve_mail(int client_sockfd, char emailname[])
{
    char buffer[1024];
    memset(buffer, 0, 1024);

    FILE *fp;
    fp = fopen(emailname, "r");

    fseek(fp, 0, 2);
    int size = ftell(fp);
    printf("%d   ", size);
    rewind(fp);
    sprintf(buffer, "%d", size);
    if(send(client_sockfd, buffer, sizeof(buffer), 0) < 0)
    {
        printf("Error during sending \n");
    }
    memset(buffer, 0, 1024);

    for(int i = 0; i < size; i++)
    {
        sprintf(buffer, "%c", fgetc(fp));
        if(send(client_sockfd, buffer, sizeof(buffer), 0) < 0)
        {
        printf("Error during sending \n");
        }
        memset(buffer, 0, 1024);
    }

    remove(emailname);
}

int main(int argc, char const *argv[]) 
{   
    int server_fd = setup_server();

    struct sockaddr_in address = get_addr();
    int addrlen = sizeof(address); 

    attach_server(server_fd, address);

    start_listening(server_fd);

    while(1)
    {
        printf("Listening for connections \n");

        char buffer[1024];
        memset(buffer, 0, 1024);

        int client_sockfd = accept_connection(server_fd, address, addrlen);

        recv(client_sockfd, buffer, sizeof(buffer), 0);
        printf("%s \n", buffer);

        if(strcmp(buffer, "SEND_MAIL") == 0)
        {
            send_mail_handler(client_sockfd);
        }

        if(strcmp(buffer, "RECV_MAIL") == 0)
        {
            recv_mail_handler(client_sockfd);
        }


    }  
    return 0;  
}