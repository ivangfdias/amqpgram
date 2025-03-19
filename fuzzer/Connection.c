#include "Connection.h"

static Grammar connection_grammar = NULL;
static Environment connection_env = NULL;
typedef enum {
  START = 10,
  START_OK = 11,
  SECURE = 20,
  SECURE_OK = 21,
  TUNE = 30,
  TUNE_OK = 31,
  OPEN = 40,
  OPEN_OK = 41,
  CLOSE = 50,
  CLOSE_OK = 51
} connection_method;

// TODO: maybe change to grammar_interface
unsigned char *shortstring_generator(int *length, Environment environment) {
  unsigned char shortstring_length = (rand() % RULE_REPETITION_INFTY) % 256;

  unsigned char *result = calloc(shortstring_length + 1, sizeof(char));

  result[0] = shortstring_length;
  int length_dummy = 0;
  for (int i = 1; i < shortstring_length + 1; i++) {
    result[i] = decode_rule("OCTET", &length_dummy, environment)[0];
  }
  *length = shortstring_length + 1;
  return result;
}

unsigned char *longstring_generator(int *length, Environment environment) {
  unsigned int string_length = (rand() % RULE_REPETITION_INFTY);
  unsigned char *result = calloc(string_length + 4, sizeof(char));

  int_in_char(result, string_length, 0, string_length + 4);
  int length_dummy = 0;
  for (int i = 4; i < string_length + 4; i++) {
    result[i] = decode_rule("OCTET", &length_dummy, environment)[0];
  }
  *length = string_length + 4;
  return result;
}

Environment setup_connection_env(Environment initial_env) {

  if (connection_env == NULL) {
    /* SETTING UP CONNECTION GRAMMAR */
    connection_grammar = generate_grammar("./abnf/grammar-connection");

    overwrite_rule_with_chaos(
        connection_grammar, initial_env, "short-string",
        new_function_grammar_entry(shortstring_generator));

    overwrite_rule_with_chaos(connection_grammar, initial_env, "long-string",
                              new_function_grammar_entry(longstring_generator));
    connection_env = new_environment(connection_grammar, initial_env);
  }
  return connection_env;
}

void cleanup_connection_env() { free_environment(connection_env); }

unsigned char *connection_start_ok(int *next_state, int *sent_packet_size,
                                   char *response_expected) {

  *next_state = 1;
  *response_expected = 1;

  Grammar mutable_grammar = create_grammar();

  Environment result_env = new_environment(mutable_grammar, connection_env);
  /* SETTING UP METHOD MESSAGE */
  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-id",
                            new_string_grammar_entry("m11-method-id"));

  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-properties",
                            new_string_grammar_entry("m11-method-properties"));

  /* GOING THROUGH METHOD SPECIFIC STUFF */
  overwrite_rule_set_length("client-properties-payload",
                            "client-properties-length", 4, mutable_grammar,
                            result_env);

  /* PLAIN AUTHENTICATION METHOD */
  overwrite_rule_with_chaos(mutable_grammar, result_env, "authcid",
                            new_string_grammar_entry("\"username\""));

  overwrite_rule_with_chaos(mutable_grammar, result_env, "passwd",
                            new_string_grammar_entry("\"password\""));

  overwrite_rule_set_length("message", "message-length", 4, mutable_grammar,
                            result_env);

  /* PREPARING METHOD PACKET */

  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar, result_env);

  unsigned char *result = decode_rule("method", sent_packet_size, result_env);

  free_environment(result_env);
  result_env = NULL;
  syslog(LOG_INFO, "C: Class 10, Method 11 [%m]");
  return result;
}

unsigned char *connection_tune_ok(int *next_state, int *sent_packet_size,
                                  char *response_expected,
                                  method_struct *method_data) {

  *next_state = 3;

  *response_expected = 0;
  /* GETTING INFORMATION FROM METHOD DATA */
  int channel_max = 0;
  int frame_max = -1;
  int received_channel_max, received_frame_max;

  if (method_data == NULL) {
    channel_max = 0xFFFF;   // max amqp short
    frame_max = 0xFFFFFFFF; // max amqp long
  } else {
    received_channel_max =
        char_in_short((unsigned char *)method_data->arguments_byte_array, 0);
    received_frame_max =
        char_in_int((unsigned char *)method_data->arguments_byte_array, 2);
    channel_max = rand() % (received_channel_max + 1); // enable overflow
    while (frame_max != 0 &&
           frame_max <
               4095) // Minimum accepted value is 4096, 0 is also accepted
      frame_max = rand() % (received_frame_max + 1); // enable overflow
  }

  Grammar mutable_grammar = create_grammar();

  Environment result_env = calloc(1, sizeof(Environment));
  result_env->grammar = mutable_grammar;
  result_env->next = connection_env;
  /* SETTING UP METHOD MESSAGE */
  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-id",
                            new_string_grammar_entry("m31-method-id"));

  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-properties",
                            new_string_grammar_entry("m31-method-properties"));

  /* GOING THROUGH METHOD SPECIFIC STUFF */
  unsigned char *frame_max_char = calloc(4, sizeof(char));
  int_in_char(frame_max_char, frame_max, 0, 4);
  overwrite_rule_with_chaos(
      mutable_grammar, result_env, "frame-max",
      new_byte_array_grammar_entry((char *)frame_max_char, 4));
  unsigned char *channel_max_char = calloc(2, sizeof(char));
  short_in_char(channel_max_char, channel_max, 0, 2);
  overwrite_rule_with_chaos(
      mutable_grammar, result_env, "channel-max",
      new_byte_array_grammar_entry((char *)channel_max_char, 2));

  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar, result_env);

  unsigned char *result = decode_rule("method", sent_packet_size, result_env);
  free_environment(result_env);

  syslog(LOG_INFO, "C: Class 10, Method 31 [%m]");
  return result;
}

unsigned char *connection_open(int *next_state, int *sent_packet_size,
                               char *response_expected) {

  *next_state = 4;
  *response_expected = 1;
  Grammar mutable_grammar = create_grammar();

  Environment result_env = new_environment(mutable_grammar, connection_env);
  /* SETTING UP METHOD MESSAGE */
  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-id",
                            new_string_grammar_entry("m40-method-id"));

  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-properties",
                            new_string_grammar_entry("m40-method-properties"));

  /* GOING THROUGH METHOD SPECIFIC STUFF */
  overwrite_rule_with_chaos(mutable_grammar, result_env, "virtual-host",
                            new_string_grammar_entry("\%x01.2F"));
  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar, result_env);

  unsigned char *result = decode_rule("method", sent_packet_size, result_env);

  syslog(LOG_INFO, "C: Class 10, Method 40 [%m]");
  return result;
}

unsigned char *connection_close(int *next_state, int *sent_packet_size,
                                char *response_expected) {
  fuzz_debug_printf("Connection close!!\n");
  *next_state = 7;
  *response_expected = 1;

  Grammar mutable_grammar = create_grammar();
  Environment result_env = new_environment(mutable_grammar, connection_env);

  /* SETTING UP METHOD MESSAGE */
  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-id",
                            new_string_grammar_entry("m50-method-id"));

  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-properties",
                            new_string_grammar_entry("m50-method-properties"));

  overwrite_rule_with_chaos(mutable_grammar, result_env, "reply-code",
                            new_string_grammar_entry("%x00.C8"));
  overwrite_rule_with_chaos(mutable_grammar, result_env, "reply-text",
                            new_string_grammar_entry("%x03.32.30.30"));
  overwrite_rule_with_chaos(mutable_grammar, result_env, "m50-class-id-val",
                            new_string_grammar_entry("%x00.00"));
  overwrite_rule_with_chaos(mutable_grammar, result_env, "m50-method-id-val",
                            new_string_grammar_entry("%x00.00"));

  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar, result_env);

  unsigned char *result = decode_rule("method", sent_packet_size, result_env);

  free_environment(result_env);

  syslog(LOG_INFO, "C: Class 10, Method 50 [%m]");
  return result;
}

unsigned char *connection_close_ok(int *next_state, int *sent_packet_size,
                                   char *response_expected) {

  *next_state = 7;
  *response_expected = 1;
  Grammar mutable_grammar = create_grammar();
  Environment result_env = new_environment(mutable_grammar, connection_env);

  /* SETTING UP METHOD MESSAGE */
  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-id",
                            new_string_grammar_entry("m51-method-id"));

  overwrite_rule_with_chaos(mutable_grammar, result_env, "method-properties",
                            new_string_grammar_entry("m51-method-properties"));
  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar, result_env);

  unsigned char *result = decode_rule("method", sent_packet_size, result_env);

  free_environment(result_env);

  syslog(LOG_INFO, "C: Class 10, Method 51 [%m]");
  return result;
}

unsigned char *connection_packet_decider(method_struct *method_data,
                                         int *next_state, int *sent_packet_size,
                                         char *response_expected) {

  if (method_data == NULL) {
    if (*next_state == 3) {

      printf("connection open\n");
      return connection_open(next_state, sent_packet_size, response_expected);

    } else if (*next_state == 5) {
      printf("Channel 0: Sending CLOSE\n");
      return connection_close(next_state, sent_packet_size, response_expected);
    }
  }
  if (method_data->class_id != CONNECTION) {
    fuzz_debug_printf(
        "Connection Packet Decider received non-Connection payload!\n");
    fuzz_debug_printf("Received class_id = %u, expected 10\n",
                      method_data->class_id);
    *next_state = 7;
    return NULL;
  }

  printf("Channel 0: Received method_id = %u\n", method_data->method_id);
  switch (method_data->method_id) {
  case START:
    fuzz_debug_printf("Channel 0: Sending START_OK\n");
    syslog(LOG_INFO, "Channel 0: Preparing START_OK");
    return connection_start_ok(next_state, sent_packet_size, response_expected);
    break;
  case TUNE:

    return connection_tune_ok(next_state, sent_packet_size, response_expected,
                              method_data);
    //    return packet_append(tune_ok, open);
  case SECURE:
  case OPEN_OK:
    fuzz_debug_printf("Connected!!!\n");
    *next_state = 5;
    *response_expected = 0;
    return NULL;
  case CLOSE:
    return connection_close_ok(next_state, sent_packet_size, response_expected);
  case CLOSE_OK:
    *next_state = 8;
    *response_expected = 0;
    return NULL;
  default:
    printf("[ERROR] Not Implemented\n");
    *next_state = 0;
    return NULL;
    break;
  }
}
