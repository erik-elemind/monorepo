set -x

gcc -c \
 main.c \
 data_log_parse.c \
 cobs_stream.c \
 ../../heatshrink/heatshrink_decoder.c \
 ../../interface/cobs.c \
 -I ../../heatshrink/ \
 -I ../../interface/ \
 -I ../../compression/ \
 -I ../../data_log/ \
 -DDL_PARSER_OFFLINE=1 \
 -DCOBS_MODE_PLAIN=1 && \
g++ -c -std=c++0x ../../compression/COBSR.cpp ../../compression/COBSR_RLE0.cpp && \
g++ -o parser main.o data_log_parse.o cobs_stream.o heatshrink_decoder.o cobs.o COBSR.o COBSR_RLE0.o && \
./parser