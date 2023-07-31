//#include <stdio.h>
//#include <stdlib.h>
//#include <x86_64-linux-gnu/sys/ioctl.h>
//#include <fcntl.h>
//#include <termios.h>
#include <unistd.h>
//
//char *portname = "/dev/ttyACM0";
//char buf[256];
//
//int main() {
//  int fd;
//  // open in non-blocking mode
//  fd = open(portname, O_RDWR | O_NOCTTY);
//
//  termios tms;
//  tcgetattr(fd, &tms);
//  // set speed
//  cfsetispeed(&tms, B9600);
//  cfsetospeed(&tms, B9600);
//  // 8 bits, no parity, no stop bits
//  tms.c_cflag &= ~PARENB;
//  tms.c_cflag &= ~CSTOPB;
//  tms.c_cflag &= ~CSIZE;
//  tms.c_cflag |= CS8;
//  tms.c_cflag &= ~CRTSCTS; // no hardware flow control
//  tms.c_cflag |= CREAD | CLOCAL; // enable receiver, ignore status lines
//  tms.c_iflag &= ~(IXON | IXOFF | IXANY); // disable io flow control, restart chars
//  tms.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
//  tms.c_oflag &= ~OPOST; // disable output processing
//
//  // wait for 12 characters to come in before read returns
//  tms.c_cc[VMIN] = 12;
//  // no minimum time to wait before read returns
//  tms.c_cc[VTIME] = 0;
//
//  // commit the options
//  tcsetattr(fd, TCSANOW, &tms);
//
//  // wait for the arduino to reset
//  usleep(1000*1000);
//  tcflush(fd, TCIFLUSH);
//  int n = read(fd, buf, 128);
//
//  printf("buffer : %s", buf);
//}

#include <stdio.h>
#include <string.h>

char serialPortFilename[] = "/dev/ttyACM0";

int main()
{
  char readBuffer[1024];
  int numBytesRead;

  FILE *serPort = fopen(serialPortFilename, "r");

  if (serPort == NULL)
  {
    printf("ERROR");
    return 0;
  }

  printf(serialPortFilename);
  printf(":\n");
  while(1)
  {
    memset(readBuffer, 0, 1024);
    fread(readBuffer, sizeof(char),1024,serPort);
    if(sizeof(readBuffer) != 0)
    {
      printf(readBuffer);
    }
  }
  return 0;
}