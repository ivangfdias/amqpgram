#include "Grammar_Interface.h"

void old_overwrite_rule_with_chaos(Grammar grammar, char *rule,
                                   grammar_entry_t *ordered_rule) {

  char **rule_array = NULL;
  char *chaos_rule = NULL;
  char *order_rule = NULL;

  char *chaos_prefix = "fuzzer-chaos-";
  char *order_prefix = "fuzzer-order-";
  int new_rule_size = strlen(chaos_prefix) + strlen(rule) + 1;

  switch (RULE_CHAOS) {
  case 0:
    grammar_insert(grammar, rule, ordered_rule);
    break;
  case 4:
    // Intentionally left blank
    break;
  case 2:
    rule_array = calloc(2, sizeof(char *));
    chaos_rule = calloc(new_rule_size, sizeof(char));
    order_rule = calloc(new_rule_size, sizeof(char));
    strcpy(chaos_rule, chaos_prefix);
    strcat(chaos_rule, rule);
    strcpy(order_rule, order_prefix);
    strcat(order_rule, rule);

    rule_array[0] = order_rule;
    rule_array[1] = chaos_rule;

    (grammar_insert(grammar, chaos_rule, grammar_lookup(grammar, rule)));
    (grammar_insert(grammar, order_rule, ordered_rule));

    (grammar_insert(grammar, rule, new_str_array_grammar_entry(rule_array, 2)));

    break;
  case 1:
  case 3:
    rule_array = calloc(4, sizeof(char *));

    chaos_rule = calloc(new_rule_size, sizeof(char));
    order_rule = calloc(new_rule_size, sizeof(char));
    strcpy(chaos_rule, chaos_prefix);
    strcat(chaos_rule, rule);
    strcpy(order_rule, order_prefix);
    strcat(order_rule, rule);

    int i;
    for (i = 0; i < 4 - RULE_CHAOS; i++) {
      rule_array[i] = order_rule;
      rule_array[4 - i - 1] = chaos_rule;
    }
    grammar_insert(grammar, chaos_rule, grammar_lookup(grammar, rule));
    grammar_insert(grammar, order_rule, ordered_rule);
    grammar_insert(grammar, rule, new_str_array_grammar_entry(rule_array, 4));
    break;
  }
}

Grammar overwrite_rule_with_chaos(Grammar target_grammar,
                                  Environment environment, char *rule,
                                  grammar_entry_t *ordered_rule) {

  if (target_grammar == NULL)
    target_grammar = create_grammar();

  char **rule_array = NULL;
  char *chaos_rule = NULL;
  char *order_rule = NULL;

  char *chaos_prefix = "fuzzer-chaos-";
  char *order_prefix = "fuzzer-order-";
  int new_rule_size = strlen(chaos_prefix) + strlen(rule) + 1;

  switch (RULE_CHAOS) {
  case 0:
    grammar_insert(target_grammar, rule, ordered_rule);
    break;
  case 4:
    // Intentionally left blank
    break;
  case 2:
    rule_array = calloc(2, sizeof(char *));
    chaos_rule = calloc(new_rule_size, sizeof(char));
    order_rule = calloc(new_rule_size, sizeof(char));
    strcpy(chaos_rule, chaos_prefix);
    strcat(chaos_rule, rule);
    strcpy(order_rule, order_prefix);
    strcat(order_rule, rule);

    rule_array[0] = order_rule;
    rule_array[1] = chaos_rule;

    (grammar_insert(target_grammar, chaos_rule,
                    environment_lookup(environment, rule)));
    (grammar_insert(target_grammar, order_rule, ordered_rule));

    (grammar_insert(target_grammar, rule,
                    new_str_array_grammar_entry(rule_array, 2)));

    break;
  case 1:
  case 3:
    rule_array = calloc(4, sizeof(char *));

    chaos_rule = calloc(new_rule_size, sizeof(char));
    order_rule = calloc(new_rule_size, sizeof(char));
    strcpy(chaos_rule, chaos_prefix);
    strcat(chaos_rule, rule);
    strcpy(order_rule, order_prefix);
    strcat(order_rule, rule);

    int i;
    for (int i = 0; i < 4; i++){
        rule_array[i] = order_rule;
    }
    for (i = 0; i < RULE_CHAOS; i++) {
      rule_array[i] = chaos_rule;
    }
    grammar_insert(target_grammar, chaos_rule,
                   environment_lookup(environment, rule));
    grammar_insert(target_grammar, order_rule, ordered_rule);
    grammar_insert(target_grammar, rule,
                   new_str_array_grammar_entry(rule_array, 4));
    break;
  }
  return target_grammar;
}

void overwrite_rule_set_length(char *rule_to_decode, char *length_rule,
                               int length_size, Grammar grammar,
                               Environment environment) {

  int decoded_rule_length = 0;
  unsigned char *decoded_rule =
      decode_rule(rule_to_decode, &decoded_rule_length, environment);

  overwrite_rule_with_chaos(grammar, environment, rule_to_decode,
                            new_grammar_entry_t(BYTE_ARRAY,
                                                (char *)decoded_rule, NULL,
                                                decoded_rule_length, NULL));

  char *decoded_length_literal = calloc(length_size, sizeof(char));
  int_in_char((unsigned char *)decoded_length_literal, decoded_rule_length, 0,
              length_size);

  overwrite_rule_with_chaos(grammar, environment, length_rule,
                            new_grammar_entry_t(BYTE_ARRAY,
                                                decoded_length_literal, NULL,
                                                length_size, NULL));
}
