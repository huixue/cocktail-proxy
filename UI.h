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
    void processUILog(MySocket *sock, string url, int serverPort, bool isSSL);
    void processUIGet(MySocket *sock, int serverPort, bool isSSL);
  protected:
    deque<UIQueueEntry *> ui_queue;
    pthread_mutex_t ui_mutex;
    void addElement(string element, int port);
    string getElement(int port, int *ret_port);
    void getLock(string msg = "");
    void putLock(string msg = "");
    string decodeUrl(string url);

    void processUILog_http(MySocket *sock, string url, int serverPort);
    void processUILog_https(MySocket *sock, string url, int serverPort);
    void processUIGet_http(MySocket *sock, int serverPort);
    void processUIGet_https(MySocket *sock, int serverPort);
    
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

    static const string FEEDBACK_PRIM_LOG_HTTPS;
    static const string FEEDBACK_REP_LOG_HTTPS;
    static const string FEEDBACK_NOOP_GET_HTTPS;
    static const string FEEDBACK_REP_GET_HTTPS_BEGIN;
    static const string FEEDBACK_REP_GET_HTTPS_MIDDLE;
    static const string FEEDBACK_REP_GET_HTTPS_END;

    static long seq_num;
        
    friend class UIQueueEntry;
};

UIMgr *uimgr();

#endif
