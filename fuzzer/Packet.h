#ifndef PACKET_H
#define PACKET_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

typedef enum {
  NONE,
  METHOD,
  HEADER,
  BODY,
  HEARTBEAT,
  PROTOCOL_HEADER
} frame_type;
typedef enum {
  CONNECTION = 10,
  CHANNEL = 20,
  EXCHANGE = 40,
  QUEUE = 50,
  BASIC = 60,
  TX = 90
} class_enum;

typedef struct {
  unsigned short class_id;
  unsigned short method_id;
  char *arguments_byte_array;
  int arguments_length;
} method_struct;
typedef struct {
  unsigned short class_id;
  unsigned short weight;
  unsigned long body_size;
  unsigned short property_flags;
  char *property_list;
  int property_list_size;
} header_struct;
typedef struct {
  char *body;
  int body_length;
} body_struct;

typedef struct {
  frame_type type;
  unsigned short channel;
  unsigned int size;
  char amqp_header[8];
  method_struct *method_payload;
  header_struct *header_payload;
  body_struct *body_payload;
} packet_struct;

void zero_packet_count();
long long int add_packet_count(int amount);
void acc_packet_count(); // add_packet_count(1);
long long int get_packet_count();

/*! Breaks an application layer packet into something that makes sense for AMQP
 */
packet_struct *break_packet(unsigned char *packet);

unsigned char *AMQP_method_frame(short channel, int size, short packet_class,
                                 short method, unsigned char *payload);

unsigned char *AMQP_header_frame(short channel, int size, long body_size,
                                 short packet_class, short weight,
                                 short property_flags, char *properties,
                                 long size_of_properties);

unsigned char *AMQP_body_frame(short channel, int size, unsigned char *payload);

unsigned char *AMQP_frame(char type, short channel, int size,
                          unsigned char *payload);

char send_packet(int connfd, unsigned char *packet, long size);

int add_string_entry(unsigned char *dest, char field_size,
                     unsigned char *field_contents, int extra_info, int index);

/*! Appends two packets, frees them, and returns a string containing the
 * appended packets.
 */
unsigned char *packet_append(unsigned char *packet1, unsigned char *packet2);
unsigned char *get_arguments(unsigned char *src);

void free_packet_struct(packet_struct *packet);

char verify_length(unsigned char *packet, int length);
#endif
