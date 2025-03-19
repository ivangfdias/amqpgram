#include "utils.h"

int fuzzing_debug = 0;
void set_fuzzing_debug(int val) { fuzzing_debug = val; }

short char_in_short(unsigned char *src, int index) {
  short result = ((short)src[index] << 0x08) + (short)src[index + 1];
  return result;
}

int char_in_int(unsigned char *src, int index) {
  int result = ((int)char_in_short(src, index) << 0x10) +
               (int)char_in_short(src, index + 2);
  return result;
}

long char_in_long(unsigned char *src, int index) {
  long result = ((long)char_in_int(src, index) << 0x20) +
                (long)char_in_int(src, index + 4);
  return result;
}

void long_in_char(unsigned char *dst, const long src, int index, size_t n) {
  if (index + 8 > n)
    return;
  memcpy(dst + index, &src, 8 * sizeof(char));
  char temp;
  size_t start = index;
  size_t end = index + 8 - 1;
  while (start < end) {
    temp = dst[start];
    dst[start] = dst[end];
    dst[end] = temp;
    start++;
    end--;
  }
}

void int_in_char(unsigned char *dst, const int src, int index, size_t n) {
  if (index + 4 > n)
    return;
  memcpy(dst + index, &src, 4 * sizeof(char));
  char temp;
  size_t start = index;
  size_t end = index + 4 - 1;
  while (start < end) {
    temp = dst[start];
    dst[start] = dst[end];
    dst[end] = temp;
    start++;
    end--;
  }
}

void short_in_char(unsigned char *dst, const short src, int index, size_t n) {
  if (index + 2 > n)
    return;
  memcpy(dst + index, &src, 2 * sizeof(char));
  char temp;
  size_t start = index;
  size_t end = index + 2 - 1;
  while (start < end) {
    temp = dst[start];
    dst[start] = dst[end];
    dst[end] = temp;
    start++;
    end--;
  }
}

unsigned char *extract_char_array_entry(unsigned char *src, int index) {
  int size = src[index];
  if (size == 0)
    return NULL;
  if (index > size)
    return NULL;
  unsigned char *result = malloc(size * sizeof(char));
  memcpy(result, src + 1 + index, size);
  return result;
}

unsigned char *extract_string_entry(unsigned char *src, int index) {
  int size = src[index];
  unsigned char *pre_result = extract_char_array_entry(src, index);
  if (pre_result == NULL)
    return NULL;
  unsigned char *result = calloc((size + 1), sizeof(char));
  memcpy(result, pre_result, size);
  free(pre_result);
  return result;
}

unsigned char *extract_field_table(unsigned char *field_table, int base_index) {
  /*
      int table_size = char_in_int(field_table, base_index);
      char* field_name;
      int field_size = 0;
      char field_type = 0;

      char* field_head = NULL;
      char* field = NULL;

      char* full_table = calloc(table_size - base_index, sizeof(char));
      int iter_offset = 0;
      int i = base_index;
      while (i < table_size){
              iter_offset = 0;
              field_name = extract_string_entry(field_table, i);
              i += strlen(field_name);
              field_type = field_table[i++];
              snprintf(field_head, "%s%c%s\n%c", field_name, 0xca, field_type,
     0xcb); //? switch(field_type){ case 'F':
                              // extract_field_table
                              // Esse é um ponto q provavelmente pode quebrar o
     broker por stack overflow porque provavelmente vai ser recursivo!! field =
     extract_field_table(field_table); case 't':
                              // boolean
                              // true = 1
                              // false = 0
                      case 'S':
                              // string
                              extract_string_entry(field_table, i);
                      default:
                              printf("New type just dropped: %c\n", type);
                              break;
              }
              strcat(full_table, "\n");

      }
*/
  return NULL;
}

char check_bit_equal(unsigned short arg1, unsigned short arg2) {
  return ((arg1 & arg2) == arg2) || ((arg1 & arg2) == arg1);
}
