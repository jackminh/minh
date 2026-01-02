#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include "init_socket.h"
#include "comm_socket.h"

int 
init_socket(int *socket_fd){
    *socket_fd  = socket(AF_INET,SOCK_STREAM,0);
    if(*socket_fd < 0){
        switch(errno){
            case EACCES:
                fprintf(stderr,"socket error#%d:%s\n",errno,EACCES_MSG);
            case EAFNOSUPPORT:
                fprintf(stderr,"socket error#%d:%s\n",errno,EAFNOSUPPORT_MSG); 
            case EMFILE:
                fprintf(stderr,"socket error#%d:%s\n",errno,EMFILE_MSG);
            case ENFILE:
                fprintf(stderr,"socket error#%d:%s\n",errno,ENFILE_MSG);
            case ENOBUFS:
                fprintf(stderr,"socket error#%d:%s\n",errno,ENOBUFS_MSG);
            case ENOMEM:
                fprintf(stderr,"socket error#%d:%s\n",errno,ENOMEM_MSG);
            case EPROTONOSUPPORT:
                fprintf(stderr,"socket error#%d:%s\n",errno,EPROTONOSUPPORT_MSG);
            case EPROTOTYPE:
                fprintf(stderr,"socket error#%d:%s\n",errno,EPROTOTYPE_MSG);
        }
        return 0;
    }   
    return 1;
}

