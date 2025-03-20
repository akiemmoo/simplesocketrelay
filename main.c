#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/socket.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<pthread.h>
#include<arpa/inet.h>

#define SVR_IP "127.0.0.1"
#define SVR_PORT 3128

typedef struct {
	int recvr;
	int sendr;
} comm_param_t;

void* perform_comm(void *args) {
	comm_param_t *params = (comm_param_t*)args;
	if (params == NULL)
		return NULL;

	printf("Communication started!\n");

	char *b = malloc(sizeof(char) * 1024);
	while(strcmp(b, "q") != 0) {
		memset(b, 0, 1024);
		int r = read(params->sendr, b, 1024);
		if (r < 0) {
			printf("Error: Data receive failed.\n");
			break;
		}

		r = write(params->recvr, b, strlen(b));
		if (r < 0) {
			printf("Error: Data forwarding failed.\n");
			break;
		} else {
			printf("Data forwarded: %s\n", b);
		}
	}
	printf("Exiting...\n");
	free(b);

	return NULL;
}

int main() {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		printf("Fatal error[%s]: Unable to open socket.\n", strerror(errno));
		close(s);
		return 1;
	}

	struct sockaddr_in s_ip;
	s_ip.sin_family = AF_INET;
	s_ip.sin_port = htons(SVR_PORT);
	
	if (inet_pton(AF_INET, SVR_IP, &s_ip.sin_addr) < 1) {
		printf("Fatal error: Unable to generate valid IP address.\n");
		close(s);
		return 1;
	}

	socklen_t s_ip_len = sizeof(s_ip);

	if (bind(s, (struct sockaddr*)&s_ip, s_ip_len) < 0) {
		printf("Fatal error[%s]: Unable to bind address and port.\n", strerror(errno));
		close(s);
		return 1;
	}

	if (listen(s, 2) < 0) {
		printf("Fatal error[%s]: Listening to port failed.\n", strerror(errno));
		close(s);
		return 1;
	}

	const char *ack = "HELLO";

	printf("Gateway opened!\nWaiting for clients...\n");

	int cl_0 = -1, cl_1 = -1;
	while(cl_0 < 0 || cl_1 < 0) {
		if (cl_0 < 0) {
			cl_0 = accept(s, (struct sockaddr*)&s_ip, &s_ip_len);
			if (write(cl_0, ack, strlen(ack)) > 0) {
				printf("First client connected!\n");
			} else {
				printf("First client connection failed!\n");
				close(cl_0);
				cl_0 = -1;
			}
			continue;
		}
		cl_1 = accept(s, (struct sockaddr*)&s_ip, &s_ip_len);
		if (write(cl_1, ack, strlen(ack)) > 0) {
			printf("Second client connected!\n");
		} else {
			printf("Second client connection failed!\n");
			close(cl_1);
			cl_1 = -1;
		}
	}
	
	pthread_t th_0, th_1;
	comm_param_t th_0_param, th_1_param;

	th_0_param.sendr = cl_0;
	th_0_param.recvr = cl_1;
	th_1_param.sendr = cl_1;
	th_1_param.recvr = cl_0;

	if (pthread_create(&th_0, NULL, perform_comm, (comm_param_t*)&th_0_param) < 0) {
		printf("Fatal error[%s]: Unable to start first thread.\n", strerror(errno));
		close(cl_0);
		close(cl_1);
		close(s);
		return 1;
	}
	if (pthread_create(&th_1, NULL, perform_comm, (comm_param_t*)&th_1_param) < 0) {
		printf("Fatal error[%s]: Unable to start second thread.\n", strerror(errno));
		close(cl_0);
		close(cl_1);
		close(s);
		return 1;
	}

	pthread_exit(NULL);

	close(cl_0);
	close(cl_1);
	close(s);
	return 0;
}
