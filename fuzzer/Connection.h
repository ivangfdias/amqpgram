#include "Grammar_Interface.h"
#include "Packet.h"
#include "utils.h"

Environment setup_connection_env(Environment initial_env);

unsigned char *connection_packet_decider(method_struct *method_data,
                                         int *sent_packet,
                                         int *sent_packet_size,
                                         char *response_expected);
unsigned char *connection_start_ok(int *next_state, int *sent_packet_size,
                                   char *response_expected);
unsigned char *connection_tune_ok(int *next_state, int *sent_packet_size,
                                  char *response_expected,
                                  method_struct *method_data);
unsigned char *connection_open(int *next_state, int *sent_packet_size,
                               char *response_expected);
unsigned char *connection_close(int *next_state, int *sent_packet_size,
                                char *response_expected);
unsigned char *connection_close_ok(int *next_state, int *sent_packet_size,
                                   char *response_expected);

void cleanup_connection_env();
