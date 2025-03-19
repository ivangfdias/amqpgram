#include "AMQP.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <unistd.h>

int PACKET_CHAOS, RULE_CHAOS = 0;

char seed_RNG(unsigned int seed) {
  if (seed == -1) {
    char *buffer = malloc(4 * sizeof(char));
    if (getrandom(buffer, 4, 0) == -1) {
      switch (errno) {
      case ENOSYS:
        printf("Note: getrandom not supported. Using time instead.\n");
        seed = time(NULL);
        break;
      default:
        return 1;
      }
    } else {
      seed = (unsigned int)char_in_int((unsigned char *)buffer, 0);
    }
    free(buffer);
  }

  srand(seed);
  printf("RNG SEED: %d\n", seed);
  syslog(LOG_NOTICE, "Fuzzer seeded with %d", seed);
  return 0;
}

int connect_to_server(char *address, int port) {

  int sockfd;
  struct sockaddr_in servaddr;

  // Cria um socket de internet do tipo stream não-bloqueante usando o protocolo
  // 0 (TCP)
  if ((sockfd = socket(AF_INET, SOCK_STREAM /* | SOCK_NONBLOCK*/, 0)) == -1) {
    perror("socket :(\n");
    exit(2);
  }

  // Inicializa as informacoes do socket
  // Com a familia (Internet)
  // Com a porta (5672) - padrao para AMQP
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port =
      htons(port); // traduz o 5672 pra entrar no negocio direito

  // Traduz um endereco ip e guarda nas informacoes do socket
  if (inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
    perror("inet_pton error");
    exit(2);
  }

  // Tenta conectar com esse socket
  //  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
  //  {

  // }

  while (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) ==
             -1 &&
         (errno == EINPROGRESS || errno == EALREADY))
    ;

  if (errno != 0 && errno != EINPROGRESS && errno != EALREADY) {
    perror("bind :(\n");
    exit(3);
  }
  return sockfd;
}

void usage(char *program_name) {

  printf("Usage: %s [address] [port] [-v[v]]\n", program_name);
}

int fuzz_with_strategy(char strat, long long int stratval, char *address,
                       int port) {
  int socketfd = -1;
  long long int currval = 0;
  int delta = 0;
  long long int printable_currval = 0;
  long long int printable_stratval = stratval;
  char *unit = NULL;

  long long int last_packets_count = 0;
  long long int next_packets_count = 0;
  zero_packet_count();
  Environment base_env = setup_protocol_env();
  setup_connection_env(base_env);

  if (strat == 't') {
    currval = time(NULL);
    printable_stratval = stratval;
    stratval = currval + stratval;
    unit = "seconds";
  } else if (strat == 'n') {
    currval = get_packet_count();
    unit = "packets";
  }
  do {

    printf("\nCreating fuzzing instance\n");
    syslog(LOG_NOTICE, "Creating fuzzing instance");
    socketfd = connect_to_server(address, port);
    delta = fuzz(socketfd, strat, stratval) - currval;
    close(socketfd);
    if (delta < 0 || delta > 2 * printable_stratval) {
      syslog(LOG_ERR, "Delta went to hell: %d [%m]", delta);
      cleanup_connection_env();
      cleanup_protocol_env();
      exit(1);
    }
    next_packets_count = get_packet_count();
    if (next_packets_count == last_packets_count) {
      syslog(LOG_WARNING, "No packets exchanged during last try. Exiting.");
      cleanup_connection_env();
      cleanup_protocol_env();
      exit(1);
    }
    last_packets_count = next_packets_count;
    currval += delta;
    printable_currval += delta;
    printf("Fuzzing Strategy Value: %lld / %lld %s\n", printable_currval,
           printable_stratval, unit);
    syslog(LOG_NOTICE, "Fuzzing Strategy Value: %lld / %lld %s [%m]",
           printable_currval, printable_stratval, unit);
  } while (currval < stratval && delta < stratval);

  cleanup_connection_env();
  cleanup_protocol_env();

  return currval;
}

int main(int argc, char **argv) {

  syslog(LOG_NOTICE, "NEW FUZZING BATTERY!");
  int socketfd = -1;
  int port = 5672;
  pthread_mutex_t socket_mutex;
  pthread_t listener_thread;
  int opt;
  int chaos, verbosity = 0;
  int fuzzing_debug = 0;
  unsigned int seed = -1;

  char strat = '0';
  long long int stratval = 0;

  char *address;
  char *endptr;
  // TODO: Support arguments correctly
  // arguments:
  //      IP
  //      [Port] default 5672
  //      [Verbosity] default 0
  //      [Packet-Chaos] default 0
  //      [Rule-Chaos] default 0
  //      [seed] default comes from /dev/urandom
  // Usage: $0 address [-p port] [-v verbosity] [-P Packet-Chaos] [-R
  // Rule-Chaos] [-s seed]

  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }
  address = calloc(strlen(argv[1]) + 1, sizeof(char));
  address = strcpy(address, argv[1]);

  while ((opt = getopt(argc, argv, "hp:v::P:R:s:n:t:")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 1;
    case 'p':
      port = strtol(optarg, &endptr, 10);
      break;
    case 'v':
      verbosity = 1;
      set_fuzzing_debug(1);
      if (optarg != NULL && strcmp(optarg, "v") == 0) {
        set_grammar_decoding_debug(1);
        verbosity++;
      }
      break;
    case 'P':
    case 'R':
      chaos = strtol(optarg, &endptr, 10);
      if (chaos < 0 || chaos > 4) {
        printf("Error: Value %d invalid on option %c (expected 0-4)\n", chaos,
               opt);
        return 1;
      }
      if (opt == 'R')
        RULE_CHAOS = chaos;
      else if (opt == 'P')
        PACKET_CHAOS = chaos;
      break;

    case 's':
      seed = strtol(optarg, &endptr, 10);
      break;
    case 'n':
    case 't':
      if (strat != '0') {
        printf("ERROR: Only use -n or -t, not both\nExiting.");
        free(address);
        exit(1);
      }
      strat = opt;
      stratval = strtol(optarg, &endptr, 10);
      break;
    }
  }

  if (verbosity > 0)
    printf("Values:\n\tAddress: %s\n\tPort: %i\n\tVerbosity: %i\n\tPacket "
           "Chaos: %i\n\tRule Chaos: "
           "%i\n",
           address, port, verbosity, PACKET_CHAOS, RULE_CHAOS);
    syslog(LOG_NOTICE,"Values:\r\n\tAddress: %s\r\n\tPort: %i\r\n\tVerbosity: %i\r\n\tPacket "
           "Chaos: %i\n\tRule Chaos: "
           "%i\n",
           address, port, verbosity, PACKET_CHAOS, RULE_CHAOS);


  printf("Connecting to server...\n");

  printf("\nStarting fuzzing tests...\n");
  seed_RNG(seed);
  fuzz_with_strategy(strat, stratval, address, port);
  free(address);
  syslog(LOG_NOTICE, "Finished fuzzing battery.");
  return 0;
}
