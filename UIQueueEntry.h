#ifndef _UIQUEUEENTRY_H_
#define _UIQUEUEENTRY_H_

#include <string>

using namespace std;

//TODO: make this flexible
#define NUM_BROWSERS 3

class UIQueueEntry {
  public:
    UIQueueEntry(string element, int port, int seq_num = 0);
        //need grep UI mutex to operate
    bool alreadyGot(int port);
    void changeBitMap(int port);
    bool allGot();
    string getElementString();
    int getPort();
  protected:
    int bitmap[NUM_BROWSERS];
    string m_element;
    int m_port;
};


#endif
