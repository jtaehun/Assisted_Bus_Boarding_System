#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#define IMAGE_PORT 2000
#define TEXT_PORT 2001
#define BUFSIZE 1000000

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    int image_sock, text_sock;
    struct sockaddr_in image_addr, text_addr;
    char image_buffer[BUFSIZE];
    char text_buffer[BUFSIZE];
    socklen_t addr_size;

    // 1. 이미지 수신용 소켓 설정
    image_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (image_sock == -1) handle_error("Image socket creation failed");
    memset(&image_addr, 0, sizeof(image_addr));
    image_addr.sin_family = AF_INET;
    image_addr.sin_port = htons(IMAGE_PORT);
    image_addr.sin_addr.s_addr = INADDR_ANY; // 어떤 IP에서든 수신 허용
    if (bind(image_sock, (struct sockaddr *)&image_addr, sizeof(image_addr)) == -1) {
        handle_error("Image socket bind failed");
    }

    // 2. 텍스트 수신용 소켓 설정
    text_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (text_sock == -1) handle_error("Text socket creation failed");
    memset(&text_addr, 0, sizeof(text_addr));
    text_addr.sin_family = AF_INET;
    text_addr.sin_port = htons(TEXT_PORT);
    text_addr.sin_addr.s_addr = INADDR_ANY; // 어떤 IP에서든 수신 허용
    if (bind(text_sock, (struct sockaddr *)&text_addr, sizeof(text_addr)) == -1) {
        handle_error("Text socket bind failed");
    }

    printf("버스 기사용 수신 프로그램 시작. 데이터 대기 중...\n");

    while (1) {
        // 3. 이미지 데이터 수신
        addr_size = sizeof(image_addr);
        int image_bytes = recvfrom(image_sock, image_buffer, BUFSIZE, 0, (struct sockaddr *)&image_addr, &addr_size);
        if (image_bytes > 0) {
            FILE *received_image = fopen("received_image.jpg", "wb");
            if (received_image) {
                fwrite(image_buffer, 1, image_bytes, received_image);
                fclose(received_image);
                printf("✅ 이미지 파일 수신 및 저장 완료: received_image.jpg\n");
            }
        }
        
        // 4. 텍스트 메시지 수신
        addr_size = sizeof(text_addr);
        int text_bytes = recvfrom(text_sock, text_buffer, BUFSIZE - 1, 0, (struct sockaddr *)&text_addr, &addr_size);
        if (text_bytes > 0) {
            text_buffer[text_bytes] = '\0'; // 문자열 끝 표시
            printf("📢 메시지 수신: %s\n", text_buffer);
        }
        
        usleep(100000); // CPU 부하를 줄이기 위해 0.1초 대기
    }

    close(image_sock);
    close(text_sock);
    return 0;
}
