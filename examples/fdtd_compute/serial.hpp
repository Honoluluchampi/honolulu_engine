#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <iostream>
#include <thread>

namespace hnll {

class serial_com // serial communicator
{
  public:
    static u_ptr<serial_com> create(char* dev_name, speed_t baud_rate = B9600)
    {
      auto ret = std::make_unique<serial_com>();
      if (ret->init(dev_name, baud_rate))
        return ret;
      else
        return nullptr;
    }

    // returns false if failed
    bool init(char* dev_name, speed_t baud_rate = B9600) {
      // open serial port
      fd_ = open(dev_name, O_RDWR);
      if (fd_ < 0) {
        perror("");
        return false;
      }

      // setup termios
      struct termios tio;
      memset(&tio, 0, sizeof(tio));
      tio.c_cflag = CS8 | CLOCAL | CREAD;
      tio.c_cc[VTIME] = 0;
      // speed
      cfsetispeed(&tio, baud_rate);
      cfsetospeed(&tio, baud_rate);
      // commit
      tcsetattr(fd_, TCSANOW, &tio);

      return true;
    }

    // currently only int is implemented
    // password is for synchronization
    // must be compatible with micro_computer
    std::string read_data(uint8_t password) {
      write(fd_, &password, 1);

      char buffer[64];

      // wait for the data coming in
      while(read(fd_, buffer, 64) <= 0) {}

      return std::string(buffer);
    }

  private:
    int fd_;
};

} // namespace hnll