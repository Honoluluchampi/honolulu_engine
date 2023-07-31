#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>

#define DEV_NAME    "/dev/ttyACM0"
#define BAUD_RATE    B9600

// シリアルポートの初期化
void serial_init(int fd)
{
  struct termios tio;
  memset(&tio, 0, sizeof(tio));
  tio.c_cflag = CS8 | CLOCAL | CREAD;
  tio.c_cc[VTIME] = 0;
  // speed
  cfsetispeed(&tio, BAUD_RATE);
  cfsetospeed(&tio, BAUD_RATE);
  // commit
  tcsetattr(fd, TCSANOW, &tio);
}


/* --------------------------------------------------------------------- */
/* メイン                                                                */
/* --------------------------------------------------------------------- */

int main(int argc,char *argv[]){
  int fd;

  // open the serial port
  fd = open(DEV_NAME, O_RDWR);
  if (fd < 0){
    perror(argv[1]);
    exit(1);
  }

  serial_init(fd);

  while(1){
    int i;
    int len; // receiving data size (in byte)
    unsigned char buffer[64];

    len=read(fd, buffer, 64);
    if(len==0){
      continue;
    }
    if(len<0){
      printf("ERROR\n");
      perror("");
      exit(2);
    }

    printf("len = %d\n", len);
    printf("%s ", buffer);
    printf("\n");
  }
}

//void setup() {
//  Serial.begin(9600);
//}
//
//void loop() {
//  int value;
//  value = analogRead(A0);
//  Serial.println(value);
//
//  delay(500);
//}
