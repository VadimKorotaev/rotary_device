#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#include "../common/protocol.h"

volatile int running = 1;
int sockfd = -1;
struct termios old_termios;

void cleanup() {
    if (sockfd != -1) {
        close(sockfd);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void handle_signal(int sig) {
    running = 0;
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void set_nonblocking_terminal() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    atexit(restore_terminal);
}

void* input_thread(void *arg) {
    printf("Управление:\n");
    printf("  w - вверх\n");
    printf("  s - вниз\n");
    printf("  a - влево\n");
    printf("  d - вправо\n");
    printf("  пробел - остановка\n");
    printf("  q - выход\n\n");
    
    while (running) {
        char c = getchar();
        uint8_t direction = 0;
        
        switch (c) {
            case 'w': direction = DIR_UP; break;
            case 's': direction = DIR_DOWN; break;
            case 'a': direction = DIR_LEFT; break;
            case 'd': direction = DIR_RIGHT; break;
            case ' ': direction = 0; break;
            case 'q': 
                running = 0; 
                break;
            default:
                continue;
        }
        
        if (!running) break;
        
        if (send_control(sockfd, direction) == -1) {
            fprintf(stderr, "Ошибка отправки команды\n");
            running = 0;
            break;
        }
    }
    
    return NULL;
}

void print_coordinates(uint16_t x, uint16_t y) {
    float x_deg = x / 100.0f;
    float y_deg = y / 100.0f;
    printf("\rТекущие координаты: X=%6.1f° Y=%6.1f°", x_deg, y_deg);
    fflush(stdout);
}

int main() {
    struct sockaddr_un addr;
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    atexit(cleanup);

    set_nonblocking_terminal();
    
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(1);
    }
    
    printf("Подключено к серверу.\n");
    
    pthread_t input_tid;
    pthread_create(&input_tid, NULL, input_thread, NULL);
    
    uint8_t buffer[256];
    
    while (running) {
        int packet_size = receive_packet(sockfd, buffer, sizeof(buffer));
        if (packet_size <= 0) {
            if (running) {
                fprintf(stderr, "\nОшибка приема данных\n");
            }
            break;
        }
        
        uint8_t cmd = buffer[3];
        
        if (cmd == CMD_COORDINATES) {
            coord_packet_t *coord = (coord_packet_t*)buffer;
            uint16_t x = coord->x_coord;
            uint16_t y = coord->y_coord;
            print_coordinates(x, y);
        }
    }
    
    running = 0;
    pthread_join(input_tid, NULL);
    
    close(sockfd);
    printf("\nКлиент завершил работу.\n");
    
    return 0;
}
