#include "grammar-parser.h"
#include <stdlib.h>
#include <time.h>

#ifndef RULE_REPETITION_INFTY
#define RULE_REPETITION_INFTY 16
#endif


unsigned char *parse_elements(char *element_str, int *decode_length,
                              Environment decode_environment);

unsigned char *parse_element(char *element_str, int *decode_length,
                             Environment decode_environment);

unsigned char *decode_rule(char *rule, int *length, Environment environment);

void set_grammar_decoding_debug(int value);

// void grammar_init(char *filename, char *abnf_filename);
Grammar grammar_init(char *filename, char *abnf_filename);
