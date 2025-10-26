#include "protocol.h"
#include <stdio.h>
#include <string.h>

uint8_t calculate_crc(const uint8_t *data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc += data[i];
    }
    return crc;
}

int send_coordinates(int sockfd, uint16_t x, uint16_t y) {
    coord_packet_t packet;
    
    packet.cnt = sizeof(packet) - 3; // без CNT, CNT2 и CRC
    packet.cnt2 = 0xFF - packet.cnt;
    packet.delimiter = PACKET_DELIMITER;
    packet.cmd = CMD_COORDINATES;
    packet.x_coord = x;
    packet.y_coord = y;
    
    uint8_t crc_data[] = {
        packet.cnt,
        packet.cnt2,
        packet.delimiter,
        packet.cmd,
        (uint8_t)(packet.x_coord & 0xFF),
        (uint8_t)((packet.x_coord >> 8) & 0xFF),
        (uint8_t)(packet.y_coord & 0xFF),
        (uint8_t)((packet.y_coord >> 8) & 0xFF)
    };
    
    packet.crc = calculate_crc(crc_data, sizeof(crc_data));
    
    ssize_t sent = write(sockfd, &packet, sizeof(packet));
    return (sent == sizeof(packet)) ? 0 : -1;
}

int send_control(int sockfd, uint8_t direction) {
    control_packet_t packet;
    
    packet.cnt = sizeof(packet) - 3; // без CNT, CNT2 и CRC
    packet.cnt2 = 0xFF - packet.cnt;
    packet.delimiter = PACKET_DELIMITER;
    packet.cmd = CMD_MOVEMENT;
    packet.direction = direction;
    
    uint8_t crc_data[] = {
        packet.cnt,
        packet.cnt2,
        packet.delimiter,
        packet.cmd,
        packet.direction
    };
    
    packet.crc = calculate_crc(crc_data, sizeof(crc_data));
    
    ssize_t sent = write(sockfd, &packet, sizeof(packet));
    return (sent == sizeof(packet)) ? 0 : -1;
}

int receive_packet(int sockfd, uint8_t *buffer, size_t buffer_size) {
    if (buffer_size < 2) return -1;
    
    ssize_t read_bytes = read(sockfd, buffer, 2);
    if (read_bytes != 2) return -1;
    
    uint8_t cnt = buffer[0];
    uint8_t cnt2 = buffer[1];
    
    if ((uint8_t)(cnt + cnt2) != 0xFF) {
        return -1;
    }
    
    size_t total_size = cnt + 3;
    if (total_size > buffer_size) {
        return -1;
    }
    
    read_bytes = read(sockfd, buffer + 2, total_size - 2);
    if (read_bytes != (ssize_t)(total_size - 2)) {
        return -1;
    }
    
    if (buffer[2] != PACKET_DELIMITER) {
        return -1;
    }
    
    uint8_t calculated_crc = calculate_crc(buffer, total_size - 1);
    if (calculated_crc != buffer[total_size - 1]) {
        return -1;
    }
    
    return total_size;
}
