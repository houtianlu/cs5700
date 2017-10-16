#include "util.h"

int main(int argc, char **argv) {
  char* recieverPortInput = NULL;
  char* mode = NULL;
  char* hostNameInput = NULL;
  parseArguments(argc, argv, &recieverPortInput, NULL, &hostNameInput, &mode);
  int port = atoi(recieverPortInput);
  int prot_mode = atoi(mode);
  struct sockaddr_in serv_addr;
  struct sockaddr_in client_addr;
  struct hostent *client;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0) {
    perror("[initServer] Cannot open a UDP socket");
    exit(1);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(16055);

  client = gethostbyname(hostNameInput);
  if (client == NULL) {
    perror("[initClientStruct] ERROR, no such host");
    exit(1);
  }
  bzero((char *) &client_addr, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  bcopy((char *)client->h_addr, (char *)&client_addr.sin_addr.s_addr, client->h_length);
  client_addr.sin_port = htons(port);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("[initServer] ERROR on binding");
    exit(1);
  }
   char buffer[PACK_SIZE];
   FILE *file;
   long fileSize;
   char fileName[256];
   Frame send_frame;
   Frame recv_frame;
   int frame_id = 0;

   while(1) {
     printf("inside while loop\n");
     unsigned int addr_size = sizeof(client_addr);
     if (recvfrom(sockfd, &recv_frame, sizeof(Frame), 0, (struct sockaddr*)&client_addr, &addr_size) < 0) {
       perror("[Recieving message]Cannot recieve any packet from sender");
       exit(1);
     }
     if (recv_frame.frame_kind == '2') {
       if (recv_frame.mode != prot_mode) {
         perror("[Recieving message]Wrong mode");
         exit(1);
       }
       if ((recv_frame.seq_num != '0') || (strlen(recv_frame.packet) == 0)) {
         perror("[Recieving message]Wrong init packet!");
         continue;
       }
       memcpy(&fileSize, recv_frame.packet, sizeof(fileSize));
       memcpy(fileName, recv_frame.packet + sizeof(fileSize), 256);
       char* fileNameString = malloc(strlen(fileName) + 1 + sizeof(long));
       strcpy(fileNameString, fileName);
       fileNameString += sprintf(fileNameString, "%ld", (long)getpid());
       if (access(fileNameString, F_OK) == -1) {
         perror("[Recieving message]Cannot Initialize file in the server, resending the ack packet");
         sendAck(send_frame, recv_frame, 2, serv_addr, sockfd);
         continue;
       }
       file = fopen(fileNameString, "w");
       if (sendAck(send_frame, recv_frame, 2, serv_addr, sockfd)) {
         memset(&send_frame, 0, sizeof(send_frame));
         memset(&recv_frame, 0, sizeof(recv_frame));
       }
       frame_id++;
     }
     if (recv_frame.frame_kind == '1') {
       if (recv_frame.seq_num < frame_id) {
         sendAck(send_frame, recv_frame, 1, serv_addr, sockfd);
       } else if (recv_frame.seq_num > frame_id) {
         continue;
       } else {
         memcpy(buffer, recv_frame.packet, PACK_SIZE);
         fwrite(buffer, 1, sizeof(buffer), file);
         fileSize -= sizeof(buffer);
         if (fileSize < 0) {
           fileSize = 0;
         }
         if (fileSize == 0) {
           printf("Breaking the while loop in reciever\n");
           sendAck(send_frame, recv_frame, 3, serv_addr, sockfd);
           break;
         }
         sendAck(send_frame, recv_frame, 1, serv_addr, sockfd);
         memset(buffer, 0, sizeof(buffer));
         memset(&send_frame, 0, sizeof(send_frame));
         memset(&recv_frame, 0, sizeof(recv_frame));
         frame_id++;
       }
     }
   }
   fclose(file);
   close(sockfd);
   return 0;
}
