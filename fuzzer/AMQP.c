#include "AMQP.h"

#define IMPLEMENTED_MESSAGES 6
pthread_cond_t read_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_read = PTHREAD_MUTEX_INITIALIZER;

// In Connection this is not a problem as only the main Thread does connection
// calls. In further classes, one should use Thread Local Storage Link for
// further info: https://en.wikipedia.org/wiki/Thread-local_storage
static Grammar protocol_grammar = NULL;

static Environment protocol_env = NULL;

// Array of mutexes for channels, malloc'ed
// Array of packet_struct* for channels, malloc'ed

pthread_mutex_t *channel_mutexes = NULL;
pthread_cond_t *channel_conds = NULL;
packet_struct **channel_packets = NULL;
pthread_t *channels = NULL;
unsigned int max_created_threads = 0;
volatile sig_atomic_t is_running = 1;

enum State {
  NOOP = -1,
  None = 0,
  ConnectionStart = 1,
  ConnectionTune = 2,
  ConnectionTuned = 3,
  //  ConnectionSecure, unused
  ConnectionOpen = 4,
  Connected = 5,
  ConnectionClose = 6,
  ConnectionCloseOK = 7,
  ConnectionClosed = 8
};

enum State current_state = None;

void thread_cleanup_handler(void *arg) {
  free(arg);
  printf("Freed %p\n", arg);
}

void debug_packet_struct(packet_struct *packet) {
  printf("\nPacket type = ");
  switch (packet->type) {
  case METHOD:
    printf("METHOD");
    break;
  case HEADER:
    printf("HEADER");
    break;
  case BODY:
    printf("BODY");
    break;
  case HEARTBEAT:
    printf("HEARTBEAT");
    break;
  case NONE:
    printf("NONE");
    break;
  default:
    printf("%d", packet->type);
    break;
  }
  printf("\n");
  printf("Channel = %d\n", packet->channel);
  printf("Length = %d\n", packet->size);
  switch (packet->type) {
  case METHOD:
    printf(" Class ID: %x\n", packet->method_payload->class_id);
    printf(" Method ID: %x\n", packet->method_payload->method_id);
    for (int i = 0; i < packet->method_payload->arguments_length; i++) {
      printf("%02x ",
             (unsigned char)packet->method_payload->arguments_byte_array[i]);
      if ((i + 1) % 16 == 0)
        printf("\n");
    }
    break;
  case HEADER:
    printf("HEADER");
    break;
  case BODY:
    printf("BODY");
    break;
  case HEARTBEAT:
    printf("HEARTBEAT");
    break;
  case NONE:
    printf("NONE");
    break;
  }
  printf("\n");
}
// Necessario para os argumentos de listener
typedef struct {
  int socket_fd;                       // fd do socket
  unsigned char recvline[MAXLINE + 1]; // Pacote recebido
  int n;                               // qtd de bytes do pacote i guess?
} listener_struct;

void *listener(void *void_args) {

  is_running = 1;
  listener_struct *args = (listener_struct *)void_args;
  int sockfd = args->socket_fd;

  int n;
  int poll_ret;

  fuzz_debug_printf("Listener born\n");
  // Le a mensagem recebida no socket

  struct pollfd fds[1];
  fds[0].fd = sockfd;
  fds[0].events = POLLIN;
  int missed_polls = 0;
  while (current_state != ConnectionClosed &&
         (poll_ret = poll(fds, 1, 500)) >= 0) {

    if (poll_ret > 0 && fds[0].revents & POLLIN) {
      syslog(LOG_INFO, "POLL: detected polling [%m]");
      if ((n = recv(sockfd, args->recvline, MAXLINE, 0) > 0)) {

        if (errno == EAGAIN || errno == EWOULDBLOCK)
          continue;
        syslog(LOG_INFO, "Poll successful: %d [%m]", n);
        fuzz_debug_printf("Listener: Got something to parse\n");
        //args->recvline[n] = 0;

        // Separating necessary for when AMQP frames come in "bundles"
        int parsed_n = 0;
        while (parsed_n < n) {
          fuzz_debug_printf("Listener: Received packet. Breaking... (%d/%d)\n",
                            parsed_n, n);
          packet_struct *read_packet = break_packet(args->recvline);
	  if (read_packet->type != NONE){
              int channel = read_packet->channel;
              pthread_mutex_lock(&channel_mutexes[channel]);
              if (channel_packets[channel] != NULL) {
                // free(channel_packets[channel]);
                channel_packets[channel] = NULL;
              }

              channel_packets[channel] =
                  realloc(channel_packets[channel], sizeof(packet_struct *));
              channel_packets[channel] = read_packet;
              pthread_mutex_unlock(&channel_mutexes[channel]);
              pthread_cond_signal(&channel_conds[channel]);
              parsed_n = read_packet->size + 7;
          } else {
		  if (read_packet != NULL){
			  printf("Listener: freeing read_packet");
			  free(read_packet);
		  }
		  break;
	  }
        }
        fuzz_debug_printf("n = %d\n", n);
        args->n = n;
      } else {
        syslog(LOG_WARNING, "Received %d bytes. Closing socket [%m]", n);
        break;
      }
    } else {
      missed_polls += 1;
      if (missed_polls >= 10) {
        syslog(LOG_WARNING, "Missed 10 polls, closing connection. [%m]");
        break;
      }
    }
  }
  if (poll_ret < 0) {
    syslog(LOG_ERR, "Polling error: returned %d. [%m]", poll_ret);
    fuzz_debug_printf("Polling error.\n");
  }

  fuzz_debug_printf("Listener exiting!\n");
  for (int i = 0; i < max_created_threads; i++) {
    pthread_cancel(channels[i]);
    pthread_cond_signal(&channel_conds[i]);
    syslog(LOG_DEBUG, "Closed channel %d", i);
  }
  is_running = 0;
  syslog(LOG_DEBUG, "Set is_running to 0");
  pthread_exit(0);
}

packet_struct *wait_response(int *ext_n, listener_struct *listener_args) {

  int n;

  pthread_mutex_lock(&mutex_read);
  while ((n = listener_args->n) == 0) {
    pthread_cond_wait(&read_cond, &mutex_read);
  }

  packet_struct *packet = break_packet(listener_args->recvline);
  listener_args->n -= packet->size + 7;
  if (listener_args->n < 0)
    listener_args->n = 0;
  debug_packet_struct(packet);

  pthread_mutex_unlock(&mutex_read);

  fuzz_debug_printf("Read!\n");
  return packet;
}

unsigned char *amqp_header(enum State *next_state, int *size,
                           char *response_expected) {

  unsigned char *packet = decode_rule("amqp", size, protocol_env);
  *response_expected = 1;
  *next_state = ConnectionStart;
  syslog(LOG_INFO, "C: AMQP Header [%m]");
  return packet;
}

unsigned char *chaotic_packet_decider(packet_struct *packet,
                                      enum State *current_state_ptr,
                                      char *response_expected, int *size) {

  int roll = rand() % IMPLEMENTED_MESSAGES;

  switch (roll) {
  case 0:
    return amqp_header(current_state_ptr, size, response_expected);
    break;
  case 1:
    return connection_start_ok(current_state_ptr, size, response_expected);
    break;
  case 2:
    return connection_tune_ok(current_state_ptr, size, response_expected, NULL);
    break;
  case 3:
    return connection_open(current_state_ptr, size, response_expected);
    break;
  case 4:
    return connection_close(current_state_ptr, size, response_expected);
    break;
  case 5:
    return connection_close_ok(current_state_ptr, size, response_expected);
    break;
  }
}

unsigned char *ordered_packet_decider(packet_struct *packet,
                                      enum State *current_state_ptr,
                                      // int sockfd,
                                      char *response_expected, int *size) {
  //  int size = 0;
  unsigned char *sent_packet;
  enum State current_state = *current_state_ptr;
  enum State next_state = current_state;

  if (packet != NULL && packet->type == PROTOCOL_HEADER) {
    syslog(LOG_INFO, "S: AMQP Header");
    *current_state_ptr = None;
    *response_expected = 0;
    return NULL;
  }
  if (current_state == None) {
    fuzz_debug_printf("No packet received, sending header\n");
    sent_packet = amqp_header(&next_state, size, response_expected);
  } else {

    if (packet == NULL) {

      if (current_state == 2 || current_state == 3 || current_state == 5 ||
          current_state == 6) {
        sent_packet = connection_packet_decider(NULL, &next_state, size,
                                                response_expected);
      }
    } else {

      if (packet->type == NONE)
        return NULL;
      if (packet->type == METHOD) {
        switch (packet->method_payload->class_id) {
        case CONNECTION:
          sent_packet = connection_packet_decider(packet->method_payload,
                                                  (int *)&next_state, size,
                                                  response_expected);
          break;
        case CHANNEL:
          // Delegate
          break;
        case EXCHANGE:
          // Delegate
          break;
        case QUEUE:
          // Delegate
          break;
        case BASIC:
          // Delegate
          break;
        case TX:
          // Delegate
          break;
        default:
          fuzz_debug_printf(
              "Class not defined, invalid packet or broken packet parsing!\n");
          break;
        }
      } else if (packet->type == HEADER) {
        // Only if consuming
      } else if (packet->type == BODY) {
        // Only if consuming
      }
    }
  }

  *current_state_ptr = next_state;
  return sent_packet;
}

enum State packet_decider(packet_struct *packet, enum State current_state,
                          int sockfd, char *response_expected) {
  int size = 0;
  unsigned char *sent_packet;
  enum State next_state = current_state;

  int roll = rand() % 4 + 1;
  if (roll >= PACKET_CHAOS) {
    fuzz_debug_printf(
        "Main: Sending packet as expected (roll = %d, PACKET_CHAOS = %d)\n",
        roll, PACKET_CHAOS);
    sent_packet =
        ordered_packet_decider(packet, &next_state, response_expected, &size);
  } else {
    fuzz_debug_printf("Main: Sending packet chosen randomly\n");
    sent_packet =
        chaotic_packet_decider(packet, &next_state, response_expected, &size);
  }

  if (sent_packet != NULL) {
    fuzz_debug_printf("Sending packet...\n");
    if (is_running){
      if (!send_packet(sockfd, sent_packet, size)){
	      fuzz_debug_printf("Sent packet!\n");
      }
      else{
	      fuzz_debug_printf("Failed to deliver packet\n");
	      next_state = ConnectionClosed;
	      *response_expected = 0;
      }
    } else {
      syslog(LOG_INFO, "Cancelled sending packet: Listener stopped running. [%m]");
      fuzz_debug_printf("Cancelled sending packet: Listener stopped running.\n");
      if (sent_packet != NULL) free(sent_packet); // as send_packet would free it otherwise
      // Forcing to prevent weird state concurrence
      next_state = ConnectionClosed;
      *response_expected = 0;
    }
  }
  return next_state;
}

typedef struct {
  int channel_id;
  int sockfd;
  Environment protocol_env;
} channel_args;

thread_local char waiting_response = 0;
void *AMQP_channel_thread(void *void_channel_args) {

  pthread_cleanup_push(thread_cleanup_handler, (void *)void_channel_args);

  channel_args *args = (channel_args *)void_channel_args;
  int my_id = args->channel_id;
  int sockfd = args->sockfd; // poderia ser global se pa
  packet_struct *channel_packet = NULL;

  pthread_mutex_t *my_mutex_read = &channel_mutexes[my_id];
  pthread_cond_t *my_read_cond = &channel_conds[my_id];

  while (current_state != ConnectionClosed) {
    pthread_mutex_lock(my_mutex_read);
    fuzz_debug_printf("Channel %d: locked mutex. Waiting response = %d \n",
                      my_id, waiting_response);
    while ((channel_packet = channel_packets[my_id]) == NULL) {
      fuzz_debug_printf("Channel %d:  Waiting condition...\n", my_id);
      if (waiting_response == 0)
        break;

      pthread_cond_wait(my_read_cond, my_mutex_read);
    }
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    fuzz_debug_printf("Channel %d: Condition achieved\n", my_id);
    enum State next_state = packet_decider(channel_packet, current_state,
                                           sockfd, &waiting_response);
    // enum State next_state = ConnectionClosed;
    if (next_state != NOOP) {
      fuzz_debug_printf("Channel %d changed current_state to %d\n", my_id,
                        next_state);
      current_state = next_state; // use mutex, or only channel 0 can do it
    }
    free(channel_packets[my_id]);
    channel_packets[my_id] = NULL;

    pthread_mutex_unlock(my_mutex_read);
    fuzz_debug_printf("Channel %d: unlocked mutex \n", my_id);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
  }
  fuzz_debug_printf("Channel %d exiting!\n", my_id);
  pthread_cleanup_pop(0);
  pthread_exit(0);
}

int create_channel_thread(int sockfd, char reset) {

  static int threads_array_size = 0;
  if (reset == 1)
    threads_array_size = 0;
  int local_created_threads = max_created_threads + 1;

  void *tmp = NULL;
  if (local_created_threads > threads_array_size) {
    if (threads_array_size == 0) {
      threads_array_size += 1;
      tmp = malloc(sizeof(pthread_mutex_t) * threads_array_size);

      channel_mutexes = (pthread_mutex_t *)tmp;
      tmp = malloc(sizeof(pthread_cond_t) * threads_array_size);

      channel_conds = (pthread_cond_t *)tmp;
      tmp = malloc(sizeof(packet_struct *) * threads_array_size);
      channel_packets = (packet_struct **)tmp;
      tmp = malloc(sizeof(pthread_t) * threads_array_size);
      channels = (pthread_t *)tmp;
    } else {
      threads_array_size = threads_array_size << 1;
      tmp = reallocarray(channel_mutexes, sizeof(pthread_mutex_t),
                         threads_array_size);

      channel_mutexes = tmp;
      tmp = reallocarray(channel_mutexes, sizeof(pthread_cond_t),
                         threads_array_size);

      channel_conds = (pthread_cond_t *)tmp;
      tmp = reallocarray(channel_mutexes, sizeof(packet_struct *),
                         threads_array_size);

      channel_packets = (packet_struct **)tmp;
      tmp =
          reallocarray(channel_mutexes, sizeof(pthread_t), threads_array_size);
      channels = (pthread_t *)tmp;
    }
  }

  pthread_mutex_t novo_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t nova_cond = PTHREAD_COND_INITIALIZER;

  channel_mutexes[max_created_threads] = novo_mutex;
  channel_conds[max_created_threads] = nova_cond;
  channel_packets[max_created_threads] = NULL;

  channel_args *novo_args = malloc(1 * sizeof(channel_args));
  novo_args->channel_id = max_created_threads;
  novo_args->sockfd = sockfd;

  pthread_mutex_lock(&novo_mutex);
  pthread_create(&channels[max_created_threads], NULL, AMQP_channel_thread,
                 (void *)novo_args);
  max_created_threads = local_created_threads;

  fuzz_debug_printf("threads_array_size = %d\n", threads_array_size);
  return max_created_threads;
}

void close_threads(pthread_t listener_thread) {

  pthread_join(listener_thread, NULL);
  syslog(LOG_DEBUG, "Listener thread closed");
  printf("Main: Connection closed.\n");
  int joined_channels = 0;
  int i = 0;
  while (joined_channels < max_created_threads) {
    pthread_join(channels[i], NULL);
    joined_channels++;
    i = (i + 1) % max_created_threads;
    syslog(LOG_DEBUG, "Main: Closed %d/%d channels...\n", joined_channels,
           max_created_threads);
    printf("Main: Closed %d/%d channels...\n", joined_channels,
           max_created_threads);
  }
}

long long int get_current_time() { return (long long int)time(NULL); }

int check_strat(long long int stratval, long long int (*stratupdate)(),
                pthread_t listener_thread) {

  long long int current_val = 0;
  while ((current_val = stratupdate()) < stratval && is_running) {
    // syslog(LOG_DEBUG, "check_strat: val during loop: %d [%m]", current_val);
    if (is_running == 0) {
      syslog(LOG_DEBUG, "check_strat: remaining val on exit after loop: %lld [%m]",
      stratval - current_val  );
      close_threads(listener_thread);
      return current_val;
    }
  }

  close_threads(listener_thread);
  syslog(LOG_DEBUG, "check_strat: remaining val on exit after loop: %lld [%m]",
         stratval - current_val  );
  return current_val;
}

Environment setup_protocol_env() {
  if (protocol_grammar == NULL) {

    protocol_grammar =
        grammar_init("./abnf/grammar-spec", "./abnf/grammar-abnf");
    protocol_env = new_environment(protocol_grammar, NULL);
  }
  return protocol_env;
}

void cleanup_protocol_env() { free_environment(protocol_env); }

int fuzz(int sockfd, char strat, long long int stratval) {

  pthread_t listener_thread;
  current_state = None;
  max_created_threads = 0;

  int retval = 0;

  printf("Main: Creating listener thread...\n");
  // Create listener thread
  listener_struct *listener_args = malloc(sizeof(listener_struct));

  listener_args->socket_fd = sockfd;
  listener_args->n = 0;

  is_running = 1;
  pthread_create(&listener_thread, NULL, listener, listener_args);
  // Create Channel 0 thread
  printf("Main: Creating Connection channel thread\n");
  create_channel_thread(sockfd, 1);
  pthread_cond_signal(&channel_conds[0]);

  if (strat == 't') {
    retval = check_strat(stratval, &get_current_time, listener_thread);
  }
  if (strat == 'n') {
    retval = check_strat(stratval, &get_packet_count, listener_thread);
  }
  if (retval > 0 || is_running == 0)
    close_threads(listener_thread);

  free(listener_args);
  printf("Main: Ended\n");

  return retval;
}
