#include <stdio.h>
#include <stdlib.h>
#include "init_socket.h"
#include "comm_socket.h"
#include "handler_pars.h"


int 
main(int argc, char **argv){
    server_addr_config config = {
        .host = NULL,
        .port = NULL,
        .daemon_mode = 0,
        .verbose = 0,
        .config_file = NULL,
        .max_connections = 1024
    };
    /*init config*/
    init_config(&config);
    /*parse paraments*/
    handler_pars(argc,argv,&config); 
    /*validate paraments*/
    if(validate_args(&config) < 0){
        print_usage(PROGRAM_NAME);
        exit(EXIT_FAILURE);
    }
    /*create socket*/
    int socket_fd;
    if(init_socket(&socket_fd)==0){
        exit(EXIT_FAILURE);
    }
    


    /*free resource */
    free_config(&config);

}