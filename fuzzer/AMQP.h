#include "Connection.h"
#include <pthread.h>
#include <signal.h>
#include <threads.h>
#include <poll.h>
#include <unistd.h>

#define MAXLINE 4096
/*! \brief Envia o cabecalho do protocolo para o socket
 */
int send_protocol_header(int sockfd);

int fuzz(int sockfd, char strat, long long int stratval);

Environment setup_protocol_env();
void cleanup_protocol_env();
