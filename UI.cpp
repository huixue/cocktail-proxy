#include "UI.h"
#include <iostream>
#include "unistd.h"
#include "assert.h"
#include "dbg.h"
#include <curl/curl.h>
using namespace std;

static UIMgr globalUIMgr;

const string UIMgr::UI_LABEL = "/bftproxylolo/";
const string UIMgr::UI_LOG_LABEL = "/bftproxylolo/log?";
const string UIMgr::UI_GET_LABEL = "/bftproxylolo/get";
const int UIMgr::PRIMARY = 8810;
const int UIMgr::REP_1 = 8808;
const int UIMgr::REP_2 = 8809;
const string UIMgr::FEEDBACK_PRIM_LOG = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_REP_LOG  = "HTTP/1.0 301\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_NOOP_GET = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_REP_GET  = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";

long UIMgr::seq_num = 0;

int UIMgr::isUIRequest(string url) {
    if(url.find(UI_LABEL) != string::npos) {
        if(url.find(UI_LOG_LABEL) != string::npos) {
            ui_dbg("UI LOG REQ \n")
            return 1;
        }
        else {
            ui_dbg("UI GET REQ \n")
            return -1;
        }
    }
    return 0;
}

void UIMgr::addElement(string element, int port) {
    getLock("ADD");
    UIQueueEntry *e = new UIQueueEntry(element, port);
    ui_queue.push_back(e);
    putLock("ADD");
}

//getElement() will NOT delete allGot element, because deque.pop_front()
//will call dtor of popped element, which is still needed
string UIMgr::getElement(int port, int *ret_port) {
    getLock("GET");
    if(ui_queue.size() <= 0) {
        ui_dbg("ui queue empty\n");
        putLock("GET");
        return string("");
    }    
    UIQueueEntry *e = ui_queue.front();
    assert(e != NULL);
    if(e->alreadyGot(port)) {
        ui_dbg("%d already got that\n", port);
        putLock("GET");
        return string("");
    }
    e->changeBitMap(port);
    *ret_port = e->getPort();
    string ret_str(e->getElementString());
    if(e->allGot()) {
        ui_dbg("all got, pop %s\n", ret_str.c_str());
        ui_queue.pop_front();
    }
    ui_dbg("getElement exit\n");
    putLock("GET");
    return ret_str;
}

string UIMgr::decodeUrl(string url) {
    CURL *handle = curl_easy_init();
    int len;
    string ret = "";
    char *decodedUrl = curl_easy_unescape(handle, url.c_str(), url.length(), &len);
    if(decodedUrl) {
        ret = decodedUrl;
        curl_free(decodedUrl);
        curl_easy_cleanup(handle);
    }
    else {
        cerr << "decodeUrl() failed" << endl;
    }
    return ret;
}


void UIMgr::processUIGet(MySocket *sock, int serverPort) {
    int elem_port;
    string element = getElement(serverPort, &elem_port);
    string feedback = "";
    bool ret = false;
    if(elem_port != serverPort) {
        if(element.length() == 0) {
            feedback = FEEDBACK_NOOP_GET;
                //ui_dbg("NO DATA to send, ack %d\n", serverPort);
        } else {
            string decodedUrl = decodeUrl(element);
            assert(decodedUrl.length() > 0);
                //ui_dbg("decoded: %s\n", decodedUrl.c_str());
            size_t queryPos = decodedUrl.find(UI_LOG_LABEL);
            assert(queryPos >= 0);
            assert(decodedUrl.length() >= queryPos + UI_LOG_LABEL.length());
            string queryStr = decodedUrl.substr(queryPos + UI_LOG_LABEL.length());
            assert(queryStr.length() > 0);
            feedback = FEEDBACK_REP_GET + queryStr;
            ui_dbg("sending back:\n|%s|TO%d\n", feedback.c_str(), serverPort);
            
                //assert(false);
        }
            //usleep(5000000);
        usleep(50000);

        ret = sock->write_bytes(feedback);
        assert(ret);
    }
}


void UIMgr::processUILog(MySocket *sock, string url, int serverPort) {
    bool ret = false;
    if(serverPort == PRIMARY) {
        cout << "primary reporting UI " << url << endl;
            //assert(false);
        addElement(url, serverPort);
        ret = sock->write_bytes(FEEDBACK_PRIM_LOG);
        assert(ret);
    } else {
        ret = sock->write_bytes(FEEDBACK_REP_LOG);
        assert(ret);
    }
}

void UIMgr::getLock(string msg) {
    pthread_mutex_lock(&ui_mutex);
        //ui_dbg("getLock %s\n", msg.c_str());
}

void UIMgr::putLock(string msg) {
        //ui_dbg("putLock %s\n", msg.c_str());
    pthread_mutex_unlock(&ui_mutex);
}


UIMgr::UIMgr() {
    pthread_mutex_init(&ui_mutex, NULL);
}

UIMgr::~UIMgr() {
    pthread_mutex_destroy(&ui_mutex);
}
UIMgr *uimgr() {
    return &globalUIMgr;
}
