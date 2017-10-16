#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define PACK_SIZE 10
#define PORT_FLAG_PATTERN "p:m:f:h:"
#define MAX_RETRY 5

typedef struct frame{
  char frame_kind;
  int seq_num;
  int ack;
  int mode;
  char packet[PACK_SIZE];
}Frame;

void parseArguments(int argc, char* argv[], char** port, char** fileName, char** hostName, char** mode);
int sendAck(Frame send_frame, Frame recv_frame, int frame_kind, struct sockaddr_in serv_addr, int sockfd);
void sendPacket(Frame send_frame, struct sockaddr_in serverAddr, char buffer[], int frame_id, int frame_kind, FILE* file, int sockfd);
int findMissing(int ack_list[], int prot_mode);
