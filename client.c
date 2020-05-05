// Client side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>  
#define SMTP_PORT 24

int setup_client()
{
    /*client_fd is the main socket with which we'll carry out the rest of the flow.
    AF_INET means we use IPv4, SOCK_STREAM refers to TCP, 0 is the default configuration.*/
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd == 0)
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
    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))                                              
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    }
    return client_fd;
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

void send_mail(int client_fd)
{
    char buffer[1024];
    memset(buffer, 0, 1024);

    strcpy(buffer, "SEND_MAIL");
    if(send(client_fd, buffer, sizeof(buffer), 0) < 0)
    {
        printf("Error during sending \n");
    }

    memset(buffer, 0, 1024);
    recv(client_fd, buffer, sizeof(buffer), 0);
    printf("%s \n", buffer );

    memset(buffer, 0, 1024);
    fgets(buffer, 1024, stdin);
    if(send(client_fd, buffer, sizeof(buffer), 0) < 0)
    {
        printf("Error during sending \n");
    }

    recv(client_fd, buffer, sizeof(buffer), 0);
    printf("%s", buffer );

    while(strcmp(buffer, "END_OF_EMAIL\n") != 0)
    {
        memset(buffer, 0, 1024);
        fgets(buffer, 1024, stdin);
        if(send(client_fd, buffer, sizeof(buffer), 0) < 0)
        {
            printf("Error during sending \n");
        }
    }
}

void recv_mail(int client_fd)
{
    char buffer[1024];
    memset(buffer, 0, 1024);

    strcpy(buffer, "RECV_MAIL");
    if(send(client_fd, buffer, sizeof(buffer), 0) < 0)
    {
        printf("Error during sending \n");
    }

    memset(buffer, 0, 1024);
    recv(client_fd, buffer, sizeof(buffer), 0);

    printf("%s", buffer);
    if(strcmp(buffer, "There are no unread emails. Have a nice day!\n")==0)
    {
        return;
    }
    printf("\n-------------------------\n");
    
    memset(buffer, 0, 1024);
    recv(client_fd, buffer, sizeof(buffer), 0);

    while(strcmp(buffer, "DONE") != 0)
    {
        int x = atoi(buffer);
        char email[x];
        memset(email, 0, x);
        for(int i = 0; i < x; i ++)
        {
            memset(buffer, 0, 1024);
            recv(client_fd, buffer, sizeof(buffer), 0);
            strcat(email, buffer);
        }
        printf("%s", email);
        printf("\n-------------------------\n");
        /*
        printf("Do you want to save this email?(y/n)");
        char ans[50];
        fgets(ans, 50, stdin);
        if(strcmp(ans, "y\n") == 0)
        {
            UP: 
            printf("Please enter a name : ");
            fgets(ans, 50, stdin);
            FILE *fp;
            fp = fopen(ans, "r");
            if(fp != NULL)
            {
                printf("File already exists.\n");
                goto UP;
            }
            fp = fopen(ans, "w");
            fprintf(fp, "%s", email);
            fclose(fp);
            printf("Email saved as %s\n", ans);
        }
        else
        {
            //do nothing
        }
        */

       //save with random strings and move on.
        memset(buffer, 0, 1024);
        recv(client_fd, buffer, sizeof(buffer), 0);
        
        
    }
    
}

int main(int argc, char *argv[]) 
{ 
    if(argc != 2)
    {
        printf("Usage : \n--send\tSend Emails\n--read\tGet unread emails\n");
        return(1);
    }
    else
    {
        int client_fd = setup_client();
    
        struct sockaddr_in address = get_addr();

        if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) != 0) { 
            printf("connection with the server failed...\n"); 
            exit(0); 
        }   
        if(strcmp(argv[1], "--send") == 0)
        {
            send_mail(client_fd);
        }
        else if(strcmp(argv[1], "--recv"))
        {
            recv_mail(client_fd);
        } 
    }
    
    
    
    
} 