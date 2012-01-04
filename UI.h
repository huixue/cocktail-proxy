#ifndef _UIMgr_H_
#define _UIMgr_H_

#include <string>
#include "MySocket.h"
#include <map>
#include <deque>
#include <pthread.h>
#include "UIQueueEntry.h"

using namespace std;

class UIMgr {
  public:
    UIMgr();
    ~UIMgr();
    int isUIRequest(string url);
    void processUILog(MySocket *sock, string url, int serverPort);
    void processUIGet(MySocket *sock, int serverPort);
  protected:
    deque<UIQueueEntry *> ui_queue;
    pthread_mutex_t ui_mutex;
    void addElement(string element, int port);
    string getElement(int port, int *ret_port);
    void getLock(string msg = "");
    void putLock(string msg = "");
    string decodeUrl(string url);
    
    
    static const string UI_LABEL;
    static const string UI_LOG_LABEL;
    static const string UI_GET_LABEL;
    static const int PRIMARY;
    static const int REP_1;
    static const int REP_2;
    static const string FEEDBACK_PRIM_LOG;
    static const string FEEDBACK_REP_LOG;
    static const string FEEDBACK_NOOP_GET;
    static const string FEEDBACK_REP_GET;
    static long seq_num;
        
    friend class UIQueueEntry;
};

UIMgr *uimgr();

#endif
