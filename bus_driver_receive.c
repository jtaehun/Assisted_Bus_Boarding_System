#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#define HOST_IP "192.168.0.20" // 버스 수신기의 IP 주소
#define IMAGE_PORT 2000        // 이미지 전송 포트
#define TEXT_PORT 2001         // 텍스트 전송 포트
#define BUFSIZE 1000000        // 이미지 데이터 버퍼 크기
#define BUTTON_PIN 76          // 키오스크 버튼과 연결된 GPIO 핀 번호

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// GPIO 핀을 입력(in) 모드로 설정
void initialize_gpio(int pin) {
    char path[50];
    sprintf(path, "/sys/class/gpio/export");
    int fd = open(path, O_WRONLY);
    if (fd < 0) handle_error("GPIO export failed");
    dprintf(fd, "%d", pin);
    close(fd);

    sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) handle_error("GPIO direction failed");
    write(fd, "in", 2);
    close(fd);
}

// GPIO 핀의 현재 상태 (눌림: 1, 안 눌림: 0) 읽기
int read_gpio_state(int pin) {
    char path[50], value_str[2];
    sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(path, O_RDONLY);
    if (fd < 0) handle_error("GPIO read failed");
    read(fd, value_str, 2);
    close(fd);
    return atoi(value_str);
}

int main(int argc, char *argv[]) {
    // 1. 이미지 파일명 입력받기: 실행 시 `socket 110.jpg`처럼 버스 번호를 인자로 받습니다.
    if (argc < 2) {
        printf("사용법: %s <버스번호.jpg>\n", argv[0]);
        return 1;
    }
    const char *image_route = argv[1];

    // 2. 네트워크 소켓 설정: UDP 통신을 위한 소켓을 생성하고, 서버 주소를 설정합니다.
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) handle_error("Socket creation failed");

    struct sockaddr_in image_addr, text_addr;
    memset(&image_addr, 0, sizeof(image_addr));
    image_addr.sin_family = AF_INET;
    image_addr.sin_port = htons(IMAGE_PORT);
    inet_pton(AF_INET, HOST_IP, &image_addr.sin_addr);

    memset(&text_addr, 0, sizeof(text_addr));
    text_addr.sin_family = AF_INET;
    text_addr.sin_port = htons(TEXT_PORT);
    inet_pton(AF_INET, HOST_IP, &text_addr.sin_addr);
    
    // 3. 하드웨어 초기화: GPIO 핀을 설정합니다.
    initialize_gpio(BUTTON_PIN);

    int prev_state = 0; // 이전 버튼 상태 저장 (0: 안 눌림, 1: 눌림)
    printf("시스템 준비 완료. 버튼 입력을 기다립니다...\n");

    // 4. 메인 루프: 버튼 상태를 계속 확인합니다.
    while (1) {
        int current_state = read_gpio_state(BUTTON_PIN);
        // 버튼이 '눌리는 순간' (이전 상태 0 -> 현재 상태 1)을 감지합니다.
        if (current_state == 1 && prev_state == 0) {
            printf("버튼이 감지되었습니다. 이미지와 메시지를 전송합니다.\n");
            
            // 이미지 파일 읽기
            char image_data[BUFSIZE] = {0,};
            FILE *file = fopen(image_route, "rb");
            if (!file) {
                printf("오류: 이미지 파일(%s)을 찾을 수 없습니다.\n", image_route);
            } else {
                size_t bytes_read = fread(image_data, 1, BUFSIZE, file);
                fclose(file);
                // 이미지 데이터 전송
                sendto(sock, image_data, bytes_read, 0, (struct sockaddr *)&image_addr, sizeof(image_addr));
            }
            
            // 텍스트 메시지 전송
            const char *text_message = "휠체어 이용객이 대기중입니다.";
            sendto(sock, text_message, strlen(text_message), 0, (struct sockaddr *)&text_addr, sizeof(text_addr));

            usleep(1000000); // 디바운싱(Debouncing) 및 중복 전송 방지를 위해 1초 대기
        }
        prev_state = current_state;
        usleep(100000); // CPU 부하를 줄이기 위해 0.1초마다 상태를 확인합니다.
    }

    close(sock);
    return 0;
}
