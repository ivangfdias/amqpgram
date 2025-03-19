#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum value_type { STRING, STRING_ARRAY, BYTE_ARRAY, FUNCTION };

typedef GHashTable *Grammar;

typedef struct EnvNode {
  Grammar grammar;
  struct EnvNode *next;
} *Environment;

typedef unsigned char*(* gen_function)(int*, Environment);

typedef struct {
  enum value_type entry_type;
  char *str_entry;
  char **str_array_entry;
  int array_size;
  gen_function generator_function;
} grammar_entry_t;



/*! \brief Constructs a new grammar_entry_t
 *
 * The default values are:
 *      value_type STRING
 *      str_entry NULL
 *      str_array_entry NULL
 *      array_size 0
 *      generator_function NULL
 *
 * \param [in] entry_type STRING or STRING_ARRAY as defined by value_type
 * \param [in] str_entry String to use in the constructor. Ignored if entry_type
 * == STRING_ARRAY \param [in] str_array_entry String array to use in the
 * constructor. must have [array_size] length. Ignored if entry_type == STRING
 * \param [in] array_size Size of the string array. Ignored if entry_type ==
 * STRING
 * \param [in] generator_function Function which accepts an integer pointer and a Grammar, and returns an unsigned char array. Ignored unless entry_type == FUNCTION
 *
 * \return Pointer to the constructed grammar_entry_t
 */
grammar_entry_t *new_grammar_entry_t(enum value_type entry_type,
                                     char *str_entry, char **str_array_entry,
                                     int array_size,
				     gen_function generator_function);

grammar_entry_t* new_string_grammar_entry(char* str_entry);
grammar_entry_t* new_str_array_grammar_entry(char** str_array_entry, int array_size);
grammar_entry_t* new_byte_array_grammar_entry(char* byte_array_entry, int array_size);
grammar_entry_t* new_function_grammar_entry(gen_function generator_function);

/*! \brief Frees the memory used by a grammar_entry_t
 *
 * \param [in] entry The entry to be freed.
 */
void free_grammar_entry_t(grammar_entry_t *entry);

/*!\brief Finds the value for a given key in a grammar
 *
 * \param [in] grammar A hash table containing the key-value pairings
 * \param [in] key A string to be used as search term
 *
 * \return The value associated with the key in grammar, if exists
 *          NULL, otherwise
 *
 */
grammar_entry_t *grammar_lookup(Grammar grammar, char *key);

grammar_entry_t* environment_lookup(Environment environment, char* key);
/*! \brief Adds a key-value pair to a grammar, replacing the original if it
 * exists.
 *
 * The memory occupied by the old value is freed automatically if it is
 * replaced.
 *
 * \param [in] grammar The Grammar to be modified
 * \param [in] key A string containing the key of the pair
 * \param [in] value The value of the pair
 *
 * \return 1 if the pair did not exist, 0 otherwise.
 */
char grammar_insert(Grammar grammar, char *key, grammar_entry_t *value);

/*! \brief Prints a key-value pairing of a Grammar, or that the key was not
 * found.
 *
 * \param [in] grammar The Grammar to be searched
 * \param [in] key A String to be searched
 *
 */
void print_key(Grammar grammar, char *key);

/*! \brief Prints all key-value pairings of a Grammar
 *
 * \param [in] grammar The Grammar to be printed
 */
void debug_print_grammar(Grammar grammar);

/*! \brief Creates a Grammar with reference count 1 and no entries
 *
 * \return An empty Grammar
 */
Grammar create_grammar();

void free_grammar(Grammar grammar);
/*! brief Reads a file and adds its contents to a Grammar
 *
 * \param [in] grammar The Grammar to be modified
 * \param [in] grammar_filename A file which contains a grammar described in
 * Augmented Backus-Naur Form (as per RFC5234)
 *
 */
void populate_grammar(Grammar grammar, char *grammar_filename);

/*! \brief Creates and populates a Grammar with the contents of a file
 *
 * \param [in] grammar_filename A file which contains a grammar described in
 * Augmented Backus-Naur Form (as per RFC5234)
 *
 * \return A grammar with the key-value pairings of the file
 *
 */
Grammar generate_grammar(char *grammar_filename);

/*! \brief Creates a Grammar with the same key-value pairings as another. They
 * are copied by value.
 *
 * \param [in] to_copy Grammar to be copied
 *
 * \return A Grammar with the same key-value pairings as to_copy.
 *
 */
Grammar copy_grammar(Grammar to_copy);



Environment new_environment(Grammar grammar, Environment next);
void free_environment(Environment node);
