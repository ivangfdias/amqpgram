#include "../grammar/packet-generator.h"
#include "utils.h"

/*! Overwrites a rule on a grammar accounting for RULE_CHAOS
 *
 *  If RULE_CHAOS is 0, overwrites normally.
 *  If RULE_CHAOS is between 1 and 3, ordered_rule has 75%/50%/25% chance of
 * being selected when parsing rule If RULE_CHAOS is 4, is a noop.
 *
 */
Grammar overwrite_rule_with_chaos(Grammar target_grammar,
                                  Environment environment, char *rule,
                                  grammar_entry_t *ordered_rule);

void overwrite_rule_set_length(char *rule_to_decode, char *length_rule,
                               int length_size, Grammar grammar,
                               Environment environment);
