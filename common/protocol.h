#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/rotary_device.sock"
#define PACKET_DELIMITER 0x01

#define CMD_COORDINATES 0xA5
#define CMD_MOVEMENT    0x55

#define DIR_RIGHT  0x01
#define DIR_LEFT   0x02
#define DIR_DOWN   0x04
#define DIR_UP     0x08

typedef struct {
    uint8_t cnt;
    uint8_t cnt2;
    uint8_t delimiter;
    uint8_t cmd;
    uint16_t x_coord;
    uint16_t y_coord;
    uint8_t crc;
} __attribute__((packed)) coord_packet_t;

typedef struct {
    uint8_t cnt;
    uint8_t cnt2;
    uint8_t delimiter;
    uint8_t cmd;
    uint8_t direction;
    uint8_t crc;
} __attribute__((packed)) control_packet_t;

uint8_t calculate_crc(const uint8_t *data, size_t len);
int send_coordinates(int sockfd, uint16_t x, uint16_t y);
int send_control(int sockfd, uint8_t direction);
int receive_packet(int sockfd, uint8_t *buffer, size_t buffer_size);

#endif