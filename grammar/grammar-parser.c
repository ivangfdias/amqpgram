#include "grammar-parser.h"

void trim_string(char *str) {
  char *start =
      str +
      strspn(str, " \t\n\r"); // Points to the first non-whitespace character
  char *end =
      str + strlen(str) - 1; // Points to the last character of the string

  while (end > start &&
         (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
    --end; // Move the end pointer backward while it points to whitespace
  }

  *(end + 1) = '\0'; // Place the null terminator after the last non-whitespace
                     // character

  // Move the trimmed string to the start of the buffer
  if (start > str) {
    memmove(str, start,
            end - start +
                2); // +2 to include the last character and null terminator
  }
}

grammar_entry_t *new_grammar_entry_t(enum value_type entry_type,
                                     char *str_entry, char **str_array_entry,
                                     int array_size,
                                     gen_function generator_function) {

  grammar_entry_t *new = NULL;
  new = calloc(1, sizeof(grammar_entry_t));
  switch (entry_type) {
  case STRING:
    new->str_entry = calloc(strlen(str_entry) + 1, sizeof(char));
    strcpy(new->str_entry, str_entry);
    new->str_array_entry = NULL;
    new->array_size = 0;
    new->generator_function = NULL;
    break;
  case STRING_ARRAY:
    new->str_entry = NULL;
    new->str_array_entry = calloc(array_size, sizeof(char *));
    new->array_size = array_size;
    for (int i = 0; i < array_size; i++) {
      new->str_array_entry[i] =
          calloc(strlen(str_array_entry[i]) + 1, sizeof(char));
      strcpy(new->str_array_entry[i], str_array_entry[i]);
    }
    new->generator_function = NULL;
    break;
  case BYTE_ARRAY:
    new->str_entry = calloc(array_size, sizeof(char));
    new->str_array_entry = NULL;
    new->array_size = array_size;
    memcpy(new->str_entry, str_entry, array_size);
    new->generator_function = NULL;
    break;
  case FUNCTION:
    new->str_entry = NULL;
    new->str_array_entry = NULL;
    new->array_size = 0;
    new->generator_function = generator_function;
    break;
  default:
    break;
  }
  new->entry_type = entry_type;
  return new;
};

grammar_entry_t *new_string_grammar_entry(char *str_entry) {
  return new_grammar_entry_t(STRING, str_entry, NULL, 0, NULL);
}
grammar_entry_t *new_str_array_grammar_entry(char **str_array_entry,
                                             int array_size) {
  return new_grammar_entry_t(STRING_ARRAY, NULL, str_array_entry, array_size,
                             NULL);
}
grammar_entry_t *new_byte_array_grammar_entry(char *byte_array_entry,
                                              int array_size) {
  return new_grammar_entry_t(BYTE_ARRAY, byte_array_entry, NULL, array_size,
                             NULL);
}
grammar_entry_t *new_function_grammar_entry(gen_function generator_function) {
  return new_grammar_entry_t(FUNCTION, NULL, NULL, 0, generator_function);
}

void free_grammar_entry_t(grammar_entry_t *entry) {
  if (entry == NULL)
    return;
  if (entry->str_entry != NULL)
    free(entry->str_entry);
  if (entry->str_array_entry != NULL) {
    for (int i = 0; i < entry->array_size; i++)
      if (entry->str_array_entry[i] != NULL)
        free(entry->str_array_entry[i]);
    free(entry->str_array_entry);
  }
  free(entry);
  return;
}

grammar_entry_t *copy_grammar_entry_t(grammar_entry_t *dest,
                                      grammar_entry_t *src) {
  if (src == NULL)
    return NULL;
  int array_size = src->array_size;
  char *str_entry = src->str_entry;
  char **str_array_entry = src->str_array_entry;
  enum value_type entry_type = src->entry_type;

  if (dest == NULL) {
    dest = calloc(1, sizeof(grammar_entry_t));
  }

  dest->entry_type = entry_type;
  dest->array_size = array_size;

  if (str_entry != NULL) {
    if (entry_type == STRING) {
      dest->str_entry = calloc(strlen(str_entry) + 1, sizeof(char));
      strcpy(dest->str_entry, str_entry);
    } else if (entry_type == BYTE_ARRAY) {
      dest->str_entry = calloc(array_size, sizeof(char));
      memcpy(dest->str_entry, str_entry, array_size);
    }
  }
  if (str_array_entry != NULL) {
    dest->str_array_entry = calloc(array_size, sizeof(char *));
    for (int i = 0; i < array_size; i++) {
      dest->str_array_entry[i] =
          calloc(strlen(str_array_entry[i]) + 1, sizeof(char));
      strcpy(dest->str_array_entry[i], str_array_entry[i]);
    }
  }

  dest->generator_function = src->generator_function;

  return dest;
}

grammar_entry_t *grammar_lookup(Grammar grammar, char *key) {

  grammar_entry_t *ret_val =
      copy_grammar_entry_t(NULL, g_hash_table_lookup(grammar, key));

  // grammar_entry_t *ret_val = g_hash_table_lookup(grammar, key);
  return ret_val;
}

grammar_entry_t *environment_lookup(Environment environment, char *key) {
  grammar_entry_t *result = NULL;
  Environment local_environment = environment;
  while (result == NULL && local_environment != NULL) {
    result = grammar_lookup(local_environment->grammar, key);
    local_environment = local_environment->next;
  }
  // syslog(LOG_DEBUG, "end local_environment = %p rule = %s[%m]",
  // local_environment, key);

  return result;
}

char grammar_insert(Grammar grammar, char *key, grammar_entry_t *value) {
  return (char)g_hash_table_insert(grammar, key, value);
}

void *print_key_value(char *key, grammar_entry_t *value, void *user_data) {
  if (value == NULL) {
    printf("key %s not found!\n", key);
    return NULL;
  }
  if (value->entry_type == STRING) {
    printf("\"%s\" : \"%s\"\n", key, value->str_entry);
    return NULL;
  }
  if (value->entry_type == STRING_ARRAY) {
    printf("\"%s\" : {", key);
    for (int i = 0; i < value->array_size; i++) {
      printf("\"%s\"", value->str_array_entry[i]);
      if (i + 1 < value->array_size)
        printf(",\n");
    }
    printf("}\n");
  }
  if (value->entry_type == FUNCTION) {
    printf("%s : FUNCTION\n", key);
  }
  if (value->entry_type == BYTE_ARRAY) {
    printf("%s : ", key);
    for (int i = 0; i < value->array_size; i++) {
      printf("%2x", value->str_entry[i]);
    }
    printf("\n");
  }
  return NULL;
}

void print_key(Grammar grammar, char *key) {
  grammar_entry_t *ret_val = grammar_lookup(grammar, key);
  print_key_value(key, ret_val, NULL);
}

void debug_print_grammar(Grammar grammar) {
  g_hash_table_foreach(grammar, (GHFunc)print_key_value, NULL);
}

grammar_entry_t *parse_value(char *read_value) {

  char *modded_value = malloc(strlen(read_value) + 1);
  strcpy(modded_value, read_value);
  char *entry = strtok(modded_value, "/");
  grammar_entry_t *ret_val;

  if (strcmp(entry, read_value) == 0) {
    ret_val = new_grammar_entry_t(STRING, read_value, NULL, 0, NULL);
  } else {

    char **array = calloc(1, sizeof(char *));
    int size = 1;
    trim_string(entry);
    array[0] = calloc(strlen(entry) + 1, sizeof(char));
    strcpy(array[0], entry);

    entry = strtok(NULL, "/");
    while (entry != NULL) {
      trim_string(entry);
      size++;
      array = realloc(array, size * sizeof(char *));
      array[size - 1] = NULL;
      array[size - 1] = calloc(strlen(entry) + 1, sizeof(char));
      strcpy(array[size - 1], entry);
      entry = strtok(NULL, "/");
    }

    ret_val = new_grammar_entry_t(STRING_ARRAY, NULL, array, size, NULL);
  }
  free(modded_value);
  return ret_val;
}

Grammar create_grammar() {
  Grammar grammar = g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
                                          (GDestroyNotify)free_grammar_entry_t);
  return grammar;
}

void free_grammar(Grammar grammar) {
  if (grammar != NULL)
    g_hash_table_destroy(grammar);
  grammar = NULL;
}

void populate_grammar(Grammar grammar, char *grammar_filename) {

  if (grammar_filename == NULL)
    grammar_filename = "grammar-spec";
  FILE *grammar_def = fopen(grammar_filename, "r");

  char *line = calloc(2048, sizeof(char));
  while (fgets(line, 2048, grammar_def)) {

    char *to_parse = calloc(strlen(line) + 1, sizeof(char));
    strcpy(to_parse, line);
    char *to_free = to_parse;
    char *key = strtok(to_parse, "=");
    char *value = strtok(NULL, ";");

    trim_string(key);

    if (value != NULL) {

      trim_string(value);
      grammar_insert(grammar, key, parse_value(value));
      // g_hash_table_insert(grammar, key, parse_value(value));
      //  print_key(grammar, key);
    } else
      free(to_free);
  }

  free(line);
  fclose(grammar_def);
}

Grammar generate_grammar(char *grammar_filename) {

  Grammar grammar = create_grammar();

  populate_grammar(grammar, grammar_filename);

  return grammar;
}

Grammar copy_grammar(Grammar to_copy) {
  Grammar copied = g_hash_table_new_similar(to_copy);
  int keys_length = 0;
  const char **keys = (const char **)g_hash_table_get_keys_as_array(
      to_copy, (guint *)&keys_length);
  for (int i = 0; i < keys_length; i++) {
    g_hash_table_insert(copied, (char *)keys[i],
                        grammar_lookup(to_copy, (char *)keys[i]));
  }
  g_free(keys);
  return copied;
}

Environment new_environment(Grammar grammar, Environment next) {
  Environment newnode = calloc(1, sizeof(struct EnvNode));
  newnode->grammar = grammar;
  newnode->next = next;
  return newnode;
}

void free_environment(Environment node) {
  if (node == NULL)
    return;
  free_grammar(node->grammar);
  node->grammar = NULL;
  node->next = NULL;
  free(node);
}
/*
int main() {
  generate_grammar(NULL);

  g_hash_table_foreach(grammar, (GHFunc) print_key_value, NULL);
  return 0;
}*/
