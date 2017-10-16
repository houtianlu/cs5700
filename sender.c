#include "util.h"

int main(int argc, char **argv) {
  char* serverPortInput = NULL;
  char* fileNameInput = NULL;
  char* mode = NULL;
  char* hostNameInput = NULL;
  int serverPort;
  int prot_mode;
	parseArguments(argc, argv, &serverPortInput, &fileNameInput, &hostNameInput, &mode);
  serverPort = atoi(serverPortInput);
  prot_mode = atoi(mode);
  struct sockaddr_in serv_addr;
  struct sockaddr_in client_addr;
  struct hostent *server;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 60000;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0) {
    perror("[initSender] Cannot open a UDP socket");
    exit(1);
  }
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    perror("[initSender] Cannot set a UDP timeout");
    exit(1);
  }
  server = gethostbyname(hostNameInput);
  if (server == NULL) {
    perror("[initSender] ERROR, no such host");
    exit(1);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(serverPort);
  bzero((char *) &client_addr, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  client_addr.sin_port = htons(15055);
  if (bind(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0) {
    perror("[initServer] ERROR on binding");
    exit(1);
  }
  int frame_id = 0;
  Frame send_frame;
  Frame recv_frame;
  int init_flag = 0;
  FILE * file;
  file = fopen(fileNameInput, "r");
  if (file == NULL) {
    perror("Cannot open file you specified!");
    exit(1);
  }
  printf("Opened file specified\n");
  long fileSize = 0;
  fseek(file, 0, SEEK_END);
  fileSize = ftell(file);
  rewind(file);
  int ack_recv = 0;
  int expect_frame = 0;
  char buffer[PACK_SIZE] = {0};
  bool terminate = false;
  int retry = 0;
  int prev_frame_id = 0;
  while (!terminate) {
    printf("in the while loop\n");
    if (init_flag == 0) {
      printf("Initializing connection\n");
      memset(&send_frame, 0, sizeof(send_frame));
      memset(buffer, 0, sizeof(buffer));
      printf("Set frame and buffer to 0\n");
      printf("%lu\n", strlen(fileNameInput));
      char *postion = send_frame.packet;
      memcpy(send_frame.packet, &fileSize, sizeof(fileSize));
      printf("Copyed fileSize %ld\n", fileSize);
      memcpy(postion + sizeof(fileSize), fileNameInput, strlen(fileNameInput) + 1);
      printf("done copying file name\n");
      sendPacket(send_frame, serv_addr, buffer, frame_id, 2, NULL, sockfd);
      memset(buffer, 0, sizeof(buffer));
      memset(&send_frame, 0, sizeof(send_frame));
    }
    if (ack_recv == 1) {
      int i;
      for (i = 0; i < prot_mode; i++) {
        memset(buffer, 0, sizeof(buffer));
        sendPacket(send_frame, serv_addr, buffer, frame_id, 1, file, sockfd);
        frame_id++;
      }
      memset(buffer, 0, sizeof(buffer));
      memset(&send_frame, 0, sizeof(send_frame));
    }
    int j;
    int ack_list[prot_mode];
    for (j = 0; j < prot_mode; j++) {
      unsigned int addr_size = sizeof(client_addr);
      if (recvfrom(sockfd, &recv_frame, sizeof(recv_frame), 0 ,(struct sockaddr*)&client_addr, &addr_size) < 0) {
        if(frame_id == 0) {
          retry++;
          break;
        } else {
          rewind(file);
          expect_frame = findMissing(ack_list, prot_mode);
          int i;
          for (i = 0; i < expect_frame - 1; i++) {
            if(fseek(file, (long)PACK_SIZE, SEEK_CUR) < 0) {
              perror("Cannot get the expect frame");
              exit(1);
            }
          }
          if (expect_frame != -1) {
            frame_id = expect_frame;
          } else {
            frame_id -= prot_mode;
          }
          if (frame_id == prev_frame_id) {
            retry++;
          } else {
            prev_frame_id = frame_id;
          }
          ack_recv = 1;
          break;
        }
      } else if (recv_frame.frame_kind == '2' && recv_frame.seq_num == 0 && strlen(recv_frame.packet) == 0) {
        init_flag = 1;
        ack_recv = 1;
        expect_frame = recv_frame.ack;
        frame_id = expect_frame;
        break;
      } else if (recv_frame.frame_kind == '1' && recv_frame.seq_num == 0 && recv_frame.ack <= frame_id && strlen(recv_frame.packet) == 0){
  			ack_list[j] = recv_frame.ack;
  		} else if (recv_frame.frame_kind == '3' && recv_frame.seq_num == 0 && recv_frame.ack <= frame_id && strlen(recv_frame.packet) == 0){
        terminate = true;
        break;
  		} else {
        ack_recv = 0;
        perror("Recieved harmful packet");
        exit(1);
      }
      memset(&recv_frame, 0, sizeof(recv_frame));
    }
    if (retry == MAX_RETRY) {
      break;
    }
  }
  fclose(file);
  close(sockfd);
  return 0;
}
