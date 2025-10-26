#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include "../common/protocol.h"

typedef struct {
    int x; // 0-360 градусов
    int y; // 0-90 градусов
    volatile uint8_t direction;
    volatile int running;
    pthread_mutex_t mutex;
} device_state_t;

device_state_t device;
int client_socket = -1;

void cleanup() {
    unlink(SOCKET_PATH);
    if (client_socket != -1) {
        close(client_socket);
    }
}

void handle_signal(int sig) {
    device.running = 0;
}

void* movement_thread(void *arg) {
    const int speed = 1;
    const int update_interval = 100000;
    
    while (device.running) {
        int new_x = device.x;
        int new_y = device.y;
        uint8_t current_direction;
        int moved = 0;
        
        pthread_mutex_lock(&device.mutex);
        current_direction = device.direction;
        pthread_mutex_unlock(&device.mutex);
        
        if (current_direction & DIR_RIGHT && new_x < 360) {
            new_x += speed;
            if (new_x > 360) new_x = 360;
            moved = 1;
        }
        
        if (current_direction & DIR_LEFT && new_x > 0) {
            new_x -= speed;
            if (new_x < 0) new_x = 0;
            moved = 1;
        }
        
        if (current_direction & DIR_UP && new_y < 90) {
            new_y += speed;
            if (new_y > 90) new_y = 90;
            moved = 1;
        }
        
        if (current_direction & DIR_DOWN && new_y > 0) {
            new_y -= speed;
            if (new_y < 0) new_y = 0;
            moved = 1;
        }
        
        if (moved || current_direction != 0) {
            pthread_mutex_lock(&device.mutex);
            device.x = new_x;
            device.y = new_y;
            if (client_socket != -1) {
                send_coordinates(client_socket, device.x * 100, device.y * 100);
            }
            pthread_mutex_unlock(&device.mutex);
        }
        
        usleep(update_interval);
    }
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_un addr;
    
    device.x = 180;
    device.y = 45;
    device.direction = 0;
    device.running = 1;
    pthread_mutex_init(&device.mutex, NULL);
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    atexit(cleanup);
    
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(1);
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    unlink(SOCKET_PATH);
    
    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(1);
    }
    
    if (listen(server_socket, 1) == -1) {
        perror("listen");
        close(server_socket);
        exit(1);
    }
    
    printf("Сервер запущен. Ожидание подключения...\n");
    
    pthread_t move_thread;
    pthread_create(&move_thread, NULL, movement_thread, NULL);
    
    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == -1) {
        perror("accept");
        device.running = 0;
    } else {
        printf("Клиент подключен.\n");
        
        uint8_t buffer[256];
        
        while (device.running) {
            int packet_size = receive_packet(client_socket, buffer, sizeof(buffer));
            if (packet_size <= 0) {
                break;
            }
            
            uint8_t cmd = buffer[3];
            
            if (cmd == CMD_MOVEMENT) {
                control_packet_t *control = (control_packet_t*)buffer;
                
                pthread_mutex_lock(&device.mutex);
                device.direction = control->direction;
                pthread_mutex_unlock(&device.mutex);
                
                printf("Получена команда движения: 0x%02X\n", control->direction);
            }
        }
    }
    
    device.running = 0;
    pthread_join(move_thread, NULL);
    
    close(client_socket);
    close(server_socket);
    pthread_mutex_destroy(&device.mutex);
    unlink(SOCKET_PATH);
    
    printf("Сервер завершил работу.\n");
    return 0;
}
