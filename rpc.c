#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE           
#include <endian.h>
#include "rpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h> 
#include <unistd.h> 


//constant definitions
#define DECIMAL 10
#define MAX_HANDLERS 20
#define MAX_CONNECTIONS 20
#define MAX_NAME_LEN 1000
#define FAIL 0 
#define ERROR -1 
#define SAME 0
#define NUM_BYTES 4
#define NUM_TYPE 5
#define RPC_FIND "find"
#define RPC_CALL "call"
#define RESPONSE "resp"
#define IS_ABSENT -1
#define IS_NULL 0 
#define NOT_NULL 1
#define MAX_DATA2_LEN 100000

/* Create the listening socket */    
int create_socket(int port); 

/* Handle for remote function */
struct rpc_handle {
    int handle_id; 
    char name[MAX_NAME_LEN]; 
};

/* Server state */
struct rpc_server {
    int socket_fd; 
    int connection_fd[MAX_CONNECTIONS]; 
    int num_connections; 
    rpc_handle handles[MAX_HANDLERS];
    rpc_handler handlers[MAX_HANDLERS]; 
    int num_handlers; 
};
 
/* Create the listening socket */  
int create_socket(int port) {

    // Initializa address we will listen on with given port 
    struct addrinfo hints, *resquest;  
    memset(&hints, 0, sizeof(hints)); 
    hints.ai_family = AF_INET6; 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE; 

    int number = port; 
    int num_digits = 0; 
    while(number/DECIMAL > 0) {
        number = number/DECIMAL;
        num_digits += 1; 
    }
    char port_string[num_digits + 1]; 
    sprintf(port_string, "%d", port); 
    int success = getaddrinfo(NULL, port_string, &hints, &resquest);
    if (success != 0) {
        fprintf(stderr, "getaddrinfor: %s\n", gai_strerror(success)); 
    } 

    // Create socket 
    int socket_fd = socket(resquest->ai_family, resquest->ai_socktype, resquest->ai_protocol); 
    if (socket_fd < FAIL) {
        perror("socket"); 
        exit(EXIT_FAILURE); 
    }

    // Allow port to be reused 
    int enable = 1; 
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < FAIL) {
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    }

    // Bind address with the socket 
    if (bind(socket_fd, resquest->ai_addr, resquest->ai_addrlen) < 0) {
        perror("bind"); 
        exit(EXIT_FAILURE); 
    }

    freeaddrinfo(resquest); 

    return socket_fd; 
}
  
/* Initialises server state */
rpc_server *rpc_init_server(int port) {
    
    // create a socket for the given port number 
    int socket_fd = create_socket(port); 

    // unsure about the number of backlog later 
    // Up to MAX_HANDLERS client connections could queue 
    if (listen(socket_fd, MAX_HANDLERS) < FAIL) {
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }

    /*
    // Accept a connection & return a new file descriptor to communicate on 
    struct sockaddr_in client_address; 
    socklen_t size_client_address = sizeof(client_address); 
    int connection_fd = accept(socket_fd, (struct sockaddr*)&client_address, &size_client_address); 
    if (connection_fd < FAIL) {
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 
    */

    // Initialise server
    rpc_server *server = malloc(sizeof(*server)); 
    server->socket_fd = socket_fd; 
    server->num_connections = 0;
    //server->connection_fd = connection_fd; 
    //server->connection_fd = -1; 
    server->num_handlers = 0; 

    return server; 
}

/* Registers a function (mapping from name to handler) */
int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    
    // Check whether there is a function registered with same name, "name" in the server
    int is_present = 0; 
    int current; 
    for (current = 0; current < srv->num_handlers; current++) {
        if (strcmp(name, (srv->handles[current]).name) == SAME) {
            is_present = 1; 
            break;
        } 
    }

    int handle_id; 
    // There is function registered with same name, "name" in the server
    // Forget the old function and replace it with the new one 
    if (is_present) {
        strcpy(srv->handles[current].name, name);
        handle_id = srv->handles[current].handle_id;  
        srv->handlers[handle_id] = handler; 

    // There is no function registered with same name, "name" in the server 
    }else {
        rpc_handle registered_handler;
        handle_id = srv->num_handlers;
        registered_handler.handle_id = handle_id;  
        strcpy(registered_handler.name, name); 
        srv->handles[handle_id] = registered_handler; 
        srv->handlers[handle_id] = handler; 
        srv->num_handlers += 1;
    }

    //printf("handle id: %d\n", handle_id);
    return handle_id;
}

void write_string (int fd, char* data) {

    // Header: Number of bytes of data sent 
    // Even if int is stored in 8 bytes, should be able to convert to 32-bit since max is 1001
    int data_len = strlen(data) + 1;
    uint32_t num_data = data_len + 1; 
    char header[NUM_BYTES];
    uint32_t* header_cast = (uint32_t*) header; 
    *header_cast = htonl(num_data);

    write(fd, header, NUM_BYTES); 
    write(fd, data, num_data);  
}

void write_int (int fd, int integer) {

    // Header: Number of bytes of data sent 
    uint32_t num_data = NUM_BYTES; 
    char header[NUM_BYTES];
    uint32_t* header_cast = (uint32_t*) header; 
    *header_cast = htonl(num_data);

    // Data: The data sent 
    uint32_t data_sent = integer;  
    char data[NUM_BYTES]; 
    uint32_t* data_cast = (uint32_t*) data; 
    *data_cast = htonl(data_sent); 

    write(fd, header, NUM_BYTES); 
    write(fd, data, NUM_BYTES); 
}

void write_rpc_data(int fd, rpc_data* data) {
    
    // Header: Number of bytes of data sent 
    //write_int(fd, data->data1); 
    
    uint32_t num_data1 = 8; 
    char header_data1[NUM_BYTES];
    uint32_t* header_cast1 = (uint32_t*) header_data1; 
    *header_cast1 = htonl(num_data1);

    // Data: The data sent 
    uint64_t data_sent = data->data1;   
    char data1[num_data1]; 
    uint64_t* data1_cast = (uint64_t*) data1; 
    *data1_cast = htobe64(data_sent); 

    write(fd, header_data1, NUM_BYTES); 
    write(fd, data1, 8); 
    

    // Header: Number of bytes of data sent 
    uint32_t num_data = data->data2_len; 
    char header[NUM_BYTES];
    uint32_t* header_cast = (uint32_t*) header; 
    *header_cast = htonl(num_data);
    write(fd, header, NUM_BYTES);

    if (data->data2_len) {
        write(fd, data->data2, num_data);
    }else if (data->data2_len > MAX_DATA2_LEN) {
        fprintf(stderr, "Overlength error\n"); 
        exit(EXIT_FAILURE);
    }
     
}

int read_int(int fd, int num_bytes) {

    char buffer[num_bytes];
    read(fd, buffer, num_bytes); 
    int integer = ntohl(*(uint32_t*)&buffer);

    return integer; 
}

int read_int64(int fd, int num_bytes) {
    char buffer[num_bytes];
    read(fd, buffer, num_bytes); 
    int integer = be64toh(*(uint64_t*)&buffer);

    return integer;
}

/* Start serving requests */
void rpc_serve_all(rpc_server *srv) {

    struct sockaddr_in client_address; 
    socklen_t size_client_address = sizeof(client_address); 
    int connection_fd = accept(srv->socket_fd, (struct sockaddr*)&client_address, &size_client_address); 
    if (connection_fd < FAIL) {
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 
    srv->connection_fd[srv->num_connections] = connection_fd; 
    srv->num_connections += 1;

    while (1) {
        char buffer[NUM_BYTES]; 

        if (read(connection_fd, buffer, NUM_BYTES)) {
            // Read the type of message here (RPC_FIND/ RPC_CALL)
            int num_type = ntohl(*(uint32_t*)&buffer);
            char type[num_type]; 
            read(connection_fd, type, num_type); 

            if (strcmp(type, RPC_FIND) == SAME) {
                
                int len_name = read_int(connection_fd, NUM_BYTES); 
                char name[len_name]; 
                read(connection_fd, name, len_name);

                int is_present = IS_ABSENT; 
                for (int i=0; i<srv->num_handlers; i++) {
                    rpc_handle handle = srv->handles[i]; 
                    if (strcmp(handle.name, name) == SAME) {
                        is_present = handle.handle_id; 
                    }
                } 
                
                write_string(connection_fd, RESPONSE);
                write_int(connection_fd, is_present); 

            }else if (strcmp(type, RPC_CALL) == SAME) {

                rpc_data* input_data = malloc(sizeof(*input_data)); 
                int len_handle_id = read_int(connection_fd, NUM_BYTES); 
                int handle_id = read_int(connection_fd, len_handle_id); 
                int len_data1 = read_int(connection_fd, NUM_BYTES); 

                input_data->data1 = read_int64(connection_fd, len_data1); 
                int len_data2 = read_int(connection_fd, NUM_BYTES); 
                input_data->data2_len = len_data2;

                if (len_data2) {
                    input_data->data2 = malloc(len_data2);
                    read(connection_fd, input_data->data2, len_data2); 
                }else {
                    input_data->data2 = NULL; 
                }
                
                rpc_data* result = srv->handlers[handle_id](input_data); 
                if (result != NULL) {
                    write_string(connection_fd, RESPONSE); 
                    write_int(connection_fd, NOT_NULL); 
                    write_rpc_data(connection_fd, result); 
                }else {
                    write_string(connection_fd, RESPONSE); 
                    write_int(connection_fd, IS_NULL); 
                }

            }
        } 
    }
}

/* Client state */
struct rpc_client {
    int socket_fd; 
    int is_closed; 
};


/* Based on COMP30023 Computer System Practical 9 with some modifications */
/* Initialises client state */
rpc_client *rpc_init_client(char *addr, int port) {

    // Initialize address 
    struct addrinfo hints; 
    memset(&hints, 0, sizeof(hints)); 
    hints.ai_family = AF_INET6; 
    hints.ai_socktype = SOCK_STREAM; 

    // Obtain address info of server
    struct addrinfo *server_info; 
    int number = port; 
    int num_digits = 0; 
    while(number/DECIMAL > 0) {
        number = number/DECIMAL;
        num_digits += 1; 
    }
    char port_string[num_digits + 1]; 
    sprintf(port_string, "%d", port); 
    int success =  getaddrinfo(addr, port_string, &hints, &server_info); 
    if (success != FAIL) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(success)); 
        exit(EXIT_FAILURE); 
    }

    // Connect to the first valid result 
    struct addrinfo *response; 
    int socket_fd; 
    for (response = server_info; response != NULL; response = response->ai_next) {
        socket_fd = socket(response->ai_family, response->ai_socktype, response->ai_protocol); 

        if (socket_fd == ERROR) {
            continue; 
        }

        if (connect(socket_fd, response->ai_addr, response->ai_addrlen) != ERROR) {
            break; 
        }

        close(socket_fd);
    }

    if (response == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE); 
    }

    freeaddrinfo(server_info); 

    rpc_client *client = malloc(sizeof(*client)); 
    client->is_closed = 0; 
    client->socket_fd = socket_fd; 

    return client;
}

/* Finds a remote function by name */
rpc_handle *rpc_find(rpc_client *cl, char *name) {

    if (cl == NULL || name == NULL) {
        return NULL; 
    }

    int socket_fd = cl->socket_fd; 
    rpc_handle* handle;  

    // write a call message 
    write_string(socket_fd, RPC_FIND); 
    write_string(socket_fd, name); 

    // read the type of message 
    int num_type = read_int(socket_fd, NUM_BYTES); 
    char type[num_type]; 
    read(socket_fd, type, num_type); 

    if (strcmp(type, RESPONSE) == SAME) {
        int len_response = read_int(socket_fd, NUM_BYTES); 
        int handle_id = read_int(socket_fd, len_response); 

        if (handle_id == IS_ABSENT) {
            return NULL; 

        }else {
            handle= malloc(sizeof(*handle));
            handle->handle_id = handle_id; 
            strcpy(handle->name, name); 
        }

    }
    //printf("rpc_find: %d\n", handle->handle_id);
    return handle;
}

/* Calls remote function using handle */
rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {

    if (cl == NULL || h == NULL || payload == NULL) {
        return NULL; 
    }

    int socket_fd = cl->socket_fd; 
    
    write_string(socket_fd, RPC_CALL); 
    write_int(socket_fd, h->handle_id);
    write_rpc_data(socket_fd, payload); 
    
    int num_type = read_int(socket_fd, NUM_BYTES); 
    char type[num_type];
    read(socket_fd, type, num_type); 

    rpc_data* result; 

    if (strcmp(type, RESPONSE) == SAME) {
        
        int num_not_null = read_int(socket_fd, NUM_BYTES); 
        int not_null = read_int(socket_fd, num_not_null); 

        if (not_null) {
            result = malloc(sizeof(*result)); 
            int len_data1 = read_int(socket_fd, NUM_BYTES); 
            //printf("%d\n", len_data1);
            result->data1 = read_int64(socket_fd, len_data1); 
            //result->data1 = read_int(socket_fd, len_data1); 
            int len_data2 = read_int(socket_fd, NUM_BYTES); 
            result->data2_len = len_data2; 
            
            //result->data2 = NULL; 
            if (len_data2) {
                //char buffer_data2[len_data2];
                result->data2 = malloc(len_data2); 
                read(socket_fd, result->data2, len_data2); 
                //result->data2 = &buffer_data2[0]; // can change to buffer_data2?? 
            }else {
                result->data2 = NULL; 
            }

        }else {
            return NULL; 
        }
    }
    
    return result; 
}

/* Cleans up client state and closes client */
void rpc_close_client(rpc_client *cl) {

    if (cl == NULL) {
        return; 
    }
    
    close(cl->socket_fd);
    cl->is_closed = 1; 

    if (cl->is_closed) {
        free(cl);
    }
     
}

/* Frees a rpc_data struct */
void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }

    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
