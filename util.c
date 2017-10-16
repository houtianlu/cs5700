#include "util.h"

//TODO: need to check the fileName length
void parseArguments(int argc, char* argv[], char** port, char** fileName, char** hostName, char** mode) {
  if (argc > 9) {
    perror("[parseArguments] Too many input, please just specifiy the port, fileName");
    exit(1);
  }
  char opt;
  // check the possible port, if null assign the default port number
	while ((opt = getopt(argc, argv, PORT_FLAG_PATTERN)) != -1) {
		switch (opt) {
			case 'p' :
				*port = optarg;
        printf("port %s\n", optarg);
				break;
      case 'f' :
        *fileName = optarg;
        printf("fileName %s\n", optarg);
        break;
      case 'm' :
        *mode = optarg;
        printf("mode: %s\n", optarg);
        break;
      case 'h' :
        *hostName = optarg;
        printf("hostName %s\n", optarg);
        break;
			default:
				perror("Wrong input, please check the input formatting");
        exit(1);
		}
	}
  // need more information
  if (argc - optind < 0) {
		perror("[parseArguments] Need more information");
    exit(1);
	}

}

int sendAck(Frame send_frame, Frame recv_frame, int frame_kind, struct sockaddr_in serv_addr, int sockfd) {
      send_frame.seq_num = 0;
      send_frame.frame_kind = frame_kind + '0';
      send_frame.ack = recv_frame.seq_num + 1;
      if (sendto(sockfd, &send_frame, sizeof(send_frame), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[Recieving message]Cannot send information from reciever to sender");
        exit(1);
      }
      printf("send packet\n");
      return 1;
}
void sendPacket(Frame send_frame, struct sockaddr_in serverAddr, char buffer[], int frame_id, int frame_kind, FILE* file, int sockfd) {
      send_frame.seq_num = frame_id;
      send_frame.frame_kind = frame_kind + '0';
      send_frame.ack = 0;
      int nread;
      if (frame_kind == '1') {
        nread = fread(buffer, 1, PACK_SIZE, file);
        printf("In sendPacket, read %d bytes\n", nread);
        if (nread > 0) {
          memcpy(send_frame.packet, &buffer, PACK_SIZE);
        }
        if (nread < PACK_SIZE) {
          printf("In sendPacket, reach to the end of the file\n");
          frame_kind = 3;
        }
      }
      if (sendto(sockfd, &send_frame, sizeof(send_frame), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Cannot send information through socket to reciever");
        exit(1);
      }
      return;
}
int findMissing(int ack_list[], int prot_mode) {
  int bucket[prot_mode];
  memset(bucket, 0, prot_mode * sizeof(int));
  int i;
  for (i = 0; i < prot_mode; i++) {
    if (ack_list[i] == 0) {
      break;
    }
    int index = ack_list[i] % prot_mode;
    bucket[index] = ack_list[i];
  }
  for (i = 0; i < prot_mode; i++) {
    if (bucket[i] == 0) {
      if (i == 0) {
        return -1;
      } else {
        return bucket[i - 1] + 1;
      }
    }
  }
  return -1;
}
