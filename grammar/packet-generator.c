#include "packet-generator.h"
#define debug_printf(...)                                                      \
  if (grammar_decoding_debug != 0)                                             \
    printf(__VA_ARGS__);

// static Grammar grammar;
static int grammar_decoding_debug = 0;

int count_occurrences(char *string, char character) {

  int characters = 0;
  for (int i = 0; i < strlen(string); i++)
    if (string[i] == character)
      characters++;
  return characters;
}

int generate_random_terminal(char *interval, int base) {

  // TODO remove
  char *temp = strtok(interval, "-");

  int min = strtol(temp, NULL, base);
  temp = strtok(NULL, "-");
  int max = strtol(temp, NULL, base);

  int gen = (rand() % (max - min)) + min;

  return gen;
}

unsigned char get_terminal_base(char *terminal) {

  int base;
  switch (terminal[1]) {
  case 'd':
    base = 10;
    break;
  case 'x':
    base = 16;
    break;
  case 'b':
    base = 2;
    break;
  default:
    debug_printf("Invalid base!\n");
    return 0;
    break;
  }
  return base;
}

char *select_alternative_rule(grammar_entry_t *value) {
  if (value->entry_type != STRING_ARRAY || value->str_array_entry == NULL ||
      value->array_size == 0)
    return NULL;

  int gen = rand() % value->array_size;
  return value->str_array_entry[gen];
}

unsigned char *decode_terminal_symbol(char *terminal_symbol, int *length) {
  int characters = count_occurrences(terminal_symbol, '.') + 1;
  int base = get_terminal_base(terminal_symbol);

  char *ret_val = NULL;
    ret_val = calloc(characters, sizeof(char));
  char *mod_val = calloc(strlen(terminal_symbol), sizeof(char));
  strcpy(mod_val, terminal_symbol + 2);

  char *temp = strtok(mod_val, ".");

  ret_val[0] = strtol(temp, NULL, base);
  for (int i = 1; i < characters; i++) {
    temp = strtok(NULL, ".");
    ret_val[i] = strtol(temp, NULL, base);
  }
  free(mod_val);
  *length = characters;
  for (int i = 0; i < characters; i++) {
  }
  return (unsigned char *)ret_val;
}

unsigned char *decode_terminal_range(char *terminal_range, int *length) {
  int base = get_terminal_base(terminal_range);

  char *terminal_range_mod = calloc(strlen(terminal_range) + 1, sizeof(char));
  strcpy(terminal_range_mod, terminal_range);
  int generated_length = 0;
  char *savepoint;
  char *temp = strtok_r(terminal_range_mod, "-", &savepoint);
  temp = (char *)decode_terminal_symbol(temp, &generated_length);

  int output_base = base;
  for (int i = 1; i < generated_length; i++) {
    output_base *= base;
  }

  int min = 0;
  int multiplied_base = 1;
  for (int i = 0; i < generated_length; i++) {
    min += (unsigned char)temp[generated_length - i - 1] * multiplied_base;
    multiplied_base *= 256;
  }

  generated_length = 0;
  temp = strtok_r(NULL, "-", &savepoint);
  temp[-2] = '%';
  switch (base) {
  case 2:
    temp[-1] = 'b';
    break;
  case 10:
    temp[-1] = 'd';
    break;
  case 16:
    temp[-1] = 'x';
    break;
  }
  temp = temp - 2;
  temp = (char *)decode_terminal_symbol(temp, &generated_length);

  int max = 0;
  multiplied_base = 1;
  for (int i = 0; i < generated_length; i++) {
    max += ((unsigned char)temp[generated_length - i - 1]) * multiplied_base;
    multiplied_base *= 256;
  }

  unsigned int gen = (rand() % (max - min)) + min;

  int string_length = 0;
  for (int i = max; i > 1; i /= 256) {
    string_length++;
  }

  unsigned char *ret_val = calloc(string_length, sizeof(char));

  multiplied_base = 1;
  for (int i = 0; i < string_length && gen > 0; i++) {
    ret_val[string_length - i - 1] = gen % 256;
    gen /= 256;
  }

  *length = string_length;
  free(terminal_range_mod);
  return (unsigned char *)ret_val;
}

unsigned char *decode_terminal(char *grammar_value, int *length) {
  if (grammar_value == NULL || grammar_value[0] != '%')
    return NULL;

  if (count_occurrences(grammar_value, '-') == 1)
    return decode_terminal_range(grammar_value, length);
  else
    return decode_terminal_symbol(grammar_value, length);
}

unsigned char *decode_quoted(char *quoted_string, int *length) {
  char *quoted = calloc(strlen(quoted_string) - 1, sizeof(unsigned char));

  strcpy(quoted, quoted_string + 1);
  for (int i = 0; i < strlen(quoted); i++) {
    if (quoted[i] >= 0x41 && quoted[i] <= 0x5A)
      quoted[i] += 20;
    else if (quoted[i] == '\"')
      quoted[i] = 0;
  }

  *length = strlen(quoted);
  return (unsigned char *)quoted;
}

unsigned char *parse_elements(char *element_str, int *decode_length,
                              Environment decode_environment) {

  debug_printf("Starting \"%s\"\n", element_str);
  int fields = count_occurrences(element_str, ' ') + 1;
  char *element_str_mod = calloc(strlen(element_str) + 1, sizeof(char));
  strcpy(element_str_mod, element_str);

  char *savepoint;
  char *element_to_parse = strtok_r(element_str_mod, " ", &savepoint);

  unsigned char *result = NULL;
  int total_length = 0;
  unsigned char *partial_result;
  int partial_length = 0;
  int real_length = 1;
  for (int i = 0; i < fields && element_to_parse != NULL; i++) {
    partial_result =
        parse_element(element_to_parse, &partial_length, decode_environment);
    debug_printf("Back on %s\n", element_str);
    total_length += partial_length;

    if (total_length >= real_length) {
      result = realloc(result, total_length * 2);
      real_length = total_length * 2;
    }
    memcpy(result + total_length - partial_length, partial_result,
           partial_length);

    element_to_parse = strtok_r(NULL, " ", &savepoint);
    partial_length = 0;
    debug_printf("Current Length on <%s> = %d\n", element_str, total_length);
  }
  result = realloc(result, total_length);

  *decode_length = total_length;
  debug_printf("Finished \"%s\"\n", element_str);
  free(element_str_mod);
  return result;
}

unsigned char *parse_repetition(char *element_str, int *decode_length,
                                Environment decode_environment) {
  int quantity = 0;
  int min = 0;
  int max = RULE_REPETITION_INFTY;
  char *element_to_repeat;

  int end_of_first = 0;
  int end_of_last = 0;
  if (element_str[end_of_first] >= 0x30 && element_str[end_of_first] <= 0x39) {
    // calcula o mínimo
    min = (element_str[end_of_first] - 0x30);

    for (end_of_first = 1;
         element_str[end_of_first] >= 0x30 && element_str[end_of_first] <= 0x39;
         end_of_first++) {
      min = min * 10 + (int)(element_str[end_of_first] - 0x30);
    }
  }
  end_of_last = end_of_first;

  if (element_str[end_of_first] == '*') {
    end_of_last = end_of_first + 1;
    if (element_str[end_of_last] >= 0x30 && element_str[end_of_last] <= 0x39) {
      // calcula o máximo
      max = (element_str[end_of_last] - 0x30);

      for (end_of_last++;
           element_str[end_of_last] >= 0x30 && element_str[end_of_last] <= 0x39;
           end_of_last++) {
        max = max * 10 + (int)(element_str[end_of_last] - 0x30);
      }
    }
  } else {

    max = min;
  }

  quantity = rand() % (max - min + 1) + min;

  if (quantity == 0) {
    *decode_length = 0;
    return NULL;
  }

  element_to_repeat =
      calloc(strlen(element_str) - end_of_last + 1, sizeof(char));
  strcpy(element_to_repeat, element_str + end_of_last);

  debug_printf(" > > Repeating \"%s\" %d times \n", element_to_repeat,
               quantity);

  unsigned char *end_result = calloc(quantity, sizeof(char));
  unsigned char *temp_result = NULL;
  int end_length = 0;
  int temp_length, position = 0;
  int real_length = quantity;
  for (int i = 0; i < quantity; i++) {
    temp_length = 0;
    temp_result =
        parse_element(element_to_repeat, &temp_length, decode_environment);

    end_length += temp_length;
    while (position + temp_length >= real_length) {

      end_result = realloc(end_result, 2 * real_length);
      real_length = 2 * real_length;
    }
    debug_printf("Back on <%s>[%d] (%d/%d), %d times more to go\n", element_str,
                 position, end_length, real_length, quantity - (i + 1));
    memcpy(end_result + position, temp_result, temp_length);
    position += temp_length;
  }
  end_result = realloc(end_result, end_length);
  if (temp_result != NULL)
    free(temp_result);
  *decode_length = end_length;
  return end_result;
}

unsigned char *parse_element(char *element_str, int *decode_length,
                             Environment decode_environment) {

  debug_printf(" > parsing element %s\n", element_str);
  if ((element_str[0] >= 0x30 && element_str[0] <= 0x39) ||
      element_str[0] == '*') {
    return parse_repetition(element_str, decode_length, decode_environment);
  } else if (element_str[0] == '%') {
    return decode_terminal(element_str, decode_length);
  } else if (element_str[0] == '\"') {
    return decode_quoted(element_str, decode_length);
  } else if (element_str[0] == '[' || element_str[0] == ']') {
    debug_printf("TODO: parse optionals\n");
    *decode_length = 0;
    return NULL;
  }
  return decode_rule(element_str, decode_length, decode_environment);
}

unsigned char *decode_rule(char *rule, int *length, Environment environment) {
  grammar_entry_t *key_value = NULL;

  if (environment != NULL) {
    key_value = environment_lookup(environment, rule);
  } else {

    //    free_grammar_entry_t(key_value);
    *length = 0;
    return NULL;
  }

  if (key_value == NULL) {
    debug_printf("Rule %s not present in provided environment", rule);
    *length = 0;
    return NULL;
  }

  debug_printf("Decoding %s\n", rule);
  unsigned char *result = NULL;

  char *rule_to_decode = NULL;
  switch (key_value->entry_type) {
  case STRING:
    result = parse_elements(key_value->str_entry, length, environment);
    break;
  case STRING_ARRAY:
    rule_to_decode = select_alternative_rule(key_value);
    result = parse_elements(rule_to_decode, length, environment);
    break;
  case BYTE_ARRAY:
    result = calloc(key_value->array_size, sizeof(char));
    memcpy(result, key_value->str_entry, key_value->array_size);
    *length = key_value->array_size;
    debug_printf("Byte array of length %d : ", key_value->array_size);
    for (int i = 0; i < key_value->array_size; i++) {
      debug_printf("%02x", result[i]);
    }
    debug_printf("\n");

    break;
  case FUNCTION:
    result = key_value->generator_function(length, environment);
  }
  free_grammar_entry_t(key_value);
  debug_printf("Decoded %s\n", rule);
  return result;
}

Grammar grammar_init(char *filename, char *abnf_filename) {
  Grammar grammar = generate_grammar(filename);
  populate_grammar(grammar, abnf_filename);
  return grammar;
}

void set_grammar_decoding_debug(int value) { grammar_decoding_debug = value; }

/*
int main(int argc, char **argv) {
  Environment environment = new_environment(generate_grammar("foo"), NULL);
  int length = 0;
  char *command;
  if (argc > 1) {
    command = argv[1];
  } else
    command = "property-flags";

  debug_printf("Command : %s\n", command);
  unsigned char *decoded = parse_elements(command, &length, environment);
  for (int i = 0; i < length; i++) {
    debug_printf("%02x", (unsigned char)decoded[i]);
  }
  debug_printf("\n");
  debug_printf("Length = %d\n", length);

  if (length < 64) {
    for (int i = 0; i < length; i++) {
      debug_printf("%2c", (unsigned char)decoded[i]);
    }
    debug_printf("\n");
  }
    free_environment(environment);
  return 0;
}*/
