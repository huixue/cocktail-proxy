#ifndef MYSOCKETEXCEPTION_H
#define MYSOCKETEXCEPTION_H

#define MSG_SIZE 100

#include <string.h>

class MySocketException {
 public:
  MySocketException(const char *message) {
    strncpy(msg,message,MSG_SIZE-1);
  }

  const char *toString() {
    return msg;
  }

 protected:
  char msg[MSG_SIZE];
};

#endif
