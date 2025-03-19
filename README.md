# AMQPGRAM: An open source grammar-based fuzzer for the AMQP protocol

## Objective

This repository contains the source code of AMQPGRAM, which is a grammar-based fuzzer for AMQP 0-9-1 implementations.

## Limitations

For now, this only supports the Connection method (except Connection.Secure and Connection.Secure-OK).
This software only supports the 0-9-1 version of the protocol, and there are no current plans on supporting future versions.

## Usage

```bash
amqpgram <target address> [target port] [STOP_CRITERIA] [-v[v]] [-s seed] [-R rule-chaos] [-P packet-chaos]
```

## Arguments

- Target address: The address in which the broker is running.
- Target port: The port in which the broker is running. Default: 5672.
- Stop Criteria: 
    - `-t N` for elapsed seconds since start; or
    - `-n N` for amount of packets exchanged since start.
- Verbosity Levels: `-v` for 1, `-vv` for 2.
- Seed: Integer to seed the random generator. Base 10 only. Default: 0.
- Rule Chaos: Integer between 0 and 4.  See following subsection for explanation.Default: 0.
- Packet Chaos: Integer between 0 and 4. See following subsection for explanation. Default: 0.

### Explaining "Chaos"

The program utilizes the grammar described by the files inside `abnf/` to generate the packets. This grammar is not enough: information such as `short-string = OCTET *string-char` is not enough to know that the `OCTET` in question is, necessarily, the length of the `string-char` repetition. As such, some functions are implemented in order to generate not only _morphologically_ valid packets, but _synctactically_ as well.

However, generating badly formed packets is imperative to fully test the capabilities of a broker. For each value of Rule Chaos greater than 0, the chance of using the raw ABNF rule improves by 25%, up to 100% at Rule Chaos = 4.

Packet Chaos progresses the same way, but for generating packets appropriate to the state of the connection. By using a Packet Chaos of 1 or above, the fuzzer could try to initiate a connection by sending a "Connection.Close" packet, for instance.

## Requirements

- gcc
- GLib (https://gitlab.gnome.org/GNOME/glib)
- make

## Academic Information

This software is being developed as part of an Institutional Scientific Initiation Scholarship Program (PIBIC) project, and would not exist without the help and financing of the National Council for Scientific and Technological Development.
