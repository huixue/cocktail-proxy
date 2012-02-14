/*======================================================== 
** University of Illinois/NCSA 
** Open Source License 
**
** Copyright (C) 2011,The Board of Trustees of the University of 
** Illinois. All rights reserved. 
**
** Developed by: 
**
**    Research Group of Professor Sam King in the Department of Computer 
**    Science The University of Illinois at Urbana-Champaign 
**    http://www.cs.uiuc.edu/homes/kingst/Research.html 
**
** Copyright (C) Hui Xue
**
** Permission is hereby granted, free of charge, to any person obtaining a 
** copy of this software and associated documentation files (the 
** Software), to deal with the Software without restriction, including 
** without limitation the rights to use, copy, modify, merge, publish, 
** distribute, sublicense, and/or sell copies of the Software, and to 
** permit persons to whom the Software is furnished to do so, subject to 
** the following conditions: 
**
** Redistributions of source code must retain the above copyright notice, 
** this list of conditions and the following disclaimers. 
**
** Redistributions in binary form must reproduce the above copyright 
** notice, this list of conditions and the following disclaimers in the 
** documentation and/or other materials provided with the distribution. 
** Neither the names of Sam King or the University of Illinois, 
** nor the names of its contributors may be used to endorse or promote 
** products derived from this Software without specific prior written 
** permission. 
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
** IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
** ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
** SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE. 
**========================================================== 
*/

#include "UI.h"
#include <iostream>
#include "unistd.h"
#include "assert.h"
#include "dbg.h"
#include <curl/curl.h>
#include <sstream>
using namespace std;

static UIMgr globalUIMgr;

const string UIMgr::UI_LABEL = "/bftproxylolo/";
const string UIMgr::UI_LOG_LABEL = "/bftproxylolo/log?";
const string UIMgr::UI_GET_LABEL = "/bftproxylolo/get";
const int UIMgr::PRIMARY = 8810;
const int UIMgr::REP_1 = 8808;
const int UIMgr::REP_2 = 8809;
/*
const string UIMgr::FEEDBACK_PRIM_LOG = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_REP_LOG  = "HTTP/1.0 301\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_NOOP_GET = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_REP_GET  = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
*/
const string UIMgr::FEEDBACK_PRIM_LOG = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_REP_LOG  = "HTTP/1.0 301\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_NOOP_GET = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";
const string UIMgr::FEEDBACK_REP_GET  = "HTTP/1.0 200\nContent-Type: text/plain\nCache-Control: no-cache\n\n";

const string UIMgr::FEEDBACK_PRIM_LOG_HTTPS =
    "HTTP/1.0 200\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nCache-Control: no-cache\r\n\r\n<html></html>";
const string UIMgr::FEEDBACK_REP_LOG_HTTPS  =
    "HTTP/1.0 301\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nCache-Control: no-cache\r\n\r\n<html></html>";
const string UIMgr::FEEDBACK_NOOP_GET_HTTPS =
    "HTTP/1.0 200\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nCache-Control: no-cache\r\n\r\n<html></html>";
const string UIMgr::FEEDBACK_REP_GET_HTTPS_BEGIN  = "HTTP/1.0 200\r\nContent-Type: text/plain\r\nContent-Length: ";
const string UIMgr::FEEDBACK_REP_GET_HTTPS_MIDDLE  = "\r\nCache-Control: no-cache\r\n\r\n<html>";
const string UIMgr::FEEDBACK_REP_GET_HTTPS_END  = "</html>";


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

void UIMgr::processUI(MySocket *sock, string url, int serverPort, bool isSSL, int type) {
        assert(type != 0);
        if(type > 0)
                processUILog(sock, url, serverPort, isSSL);
        else
                processUIGet(sock, serverPort, isSSL);
}


void UIMgr::processUILog(MySocket *sock, string url, int serverPort, bool isSSL) {
    if(!isSSL) {
        processUILog_http(sock, url, serverPort);
    } else
        processUILog_https(sock, url, serverPort);
}
void UIMgr::processUIGet(MySocket *sock, int serverPort, bool isSSL) {
    if(!isSSL) {
        processUIGet_http(sock, serverPort);
    } else
        processUIGet_https(sock, serverPort);
}


void UIMgr::processUIGet_http(MySocket *sock, int serverPort) {
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
            assert(queryPos != string::npos);
            assert(decodedUrl.length() >= queryPos + UI_LOG_LABEL.length());
            string queryStr = decodedUrl.substr(queryPos + UI_LOG_LABEL.length());
            assert(queryStr.length() > 0);
            feedback = FEEDBACK_REP_GET + queryStr;
            ui_dbg("sending back:\n|%s|TO%d\n", feedback.c_str(), serverPort);            
                //assert(false);
        }
            //usleep(5000000);
//        usleep(50000);

        ret = sock->write_bytes(feedback);
        assert(ret);
    }
}
void UIMgr::processUIGet_https(MySocket *sock, int serverPort) {
    int elem_port;
    string element = getElement(serverPort, &elem_port);
    string feedback = "";
    bool ret = false;
    stringstream length_stream;
    if(elem_port != serverPort) {
        if(element.length() == 0) {
            feedback = FEEDBACK_NOOP_GET_HTTPS;
                //ui_dbg("NO DATA to send, ack %d\n", serverPort);
        } else {
            string decodedUrl = decodeUrl(element);
            assert(decodedUrl.length() > 0);
                //ui_dbg("decoded: %s\n", decodedUrl.c_str());
            size_t queryPos = decodedUrl.find(UI_LOG_LABEL);
            assert(queryPos != string::npos);
            assert(decodedUrl.length() >= queryPos + UI_LOG_LABEL.length());
            string queryStr = decodedUrl.substr(queryPos + UI_LOG_LABEL.length());
            assert(queryStr.length() > 0);
            
            length_stream << queryStr.length() + 13;
            
            feedback = FEEDBACK_REP_GET_HTTPS_BEGIN + length_stream.str() +
                FEEDBACK_REP_GET_HTTPS_MIDDLE + queryStr + FEEDBACK_REP_GET_HTTPS_END;
            
            ui_dbg("sending back:\n|%s|TO%d\n", feedback.c_str(), serverPort);            
                //assert(false);
        }
            //usleep(5000000);
//        usleep(50000);

        ret = sock->write_bytes(feedback);
        assert(ret);
    }
}


void UIMgr::processUILog_http(MySocket *sock, string url, int serverPort) {
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

void UIMgr::processUILog_https(MySocket *sock, string url, int serverPort) {
    bool ret = false;
    if(serverPort == PRIMARY) {
        cout << "primary reporting UI " << url << endl;
            //assert(false);
        addElement(url, serverPort);
        ret = sock->write_bytes(FEEDBACK_PRIM_LOG_HTTPS);
        assert(ret);
    } else {
        ret = sock->write_bytes(FEEDBACK_REP_LOG_HTTPS);
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
