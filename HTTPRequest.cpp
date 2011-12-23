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
** Copyright (C) Sam King
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


#include "HTTPRequest.h"

#include <iostream>
#include <string>

#include <assert.h>
#include <errno.h>

#include "dbg.h"

using namespace std;

#define CONNECT_REPLY "HTTP/1.1 200 Connection Established\r\n\r\n"

HTTPRequest::HTTPRequest(MySocket *sock, int serverPort)
{
    m_sock = sock;
    m_http = new HTTP();
    m_serverPort = serverPort;
    m_totalBytesRead = 0;
    m_totalBytesWritten = 0;
}

HTTPRequest::~HTTPRequest()
{
    delete m_http;
}

void HTTPRequest::printDebugInfo()
{
    cerr << "    isDone = " << m_http->isDone() << endl;
    cerr << "    bytesRead = " << m_totalBytesRead << endl;
    cerr << "    bytesWritte = " << m_totalBytesWritten << endl;
    cerr << "    url = " << m_http->getUrl() << endl;
}

bool HTTPRequest::readRequest()
{
    assert(!m_http->isDone());
    unsigned char buf[1024];

    int num_bytes;
    while(!m_http->isDone()) {
        num_bytes = m_sock->read(buf, sizeof(buf));
        if(num_bytes > 0) {
            onRead(buf, (unsigned int) num_bytes);
        } else {
            cerr << "socket error" << endl;
            return false;
        }
    }
//    httpreq_dbg("req: %s\n", getUrl().c_str());
//    httpreq_dbg("req: %s\n", m_http->getProxyRequest().c_str());
    
    return true;
}


void HTTPRequest::onRead(const unsigned char *buffer, unsigned int len)
{
    m_totalBytesRead += len;

    unsigned int bytesRead = 0;
    assert(len > 0);

    while(bytesRead < len) {
        assert(!m_http->isDone());
        int ret = m_http->addData(buffer + bytesRead, len - bytesRead);
        assert(ret > 0);
        bytesRead += ret;
        
        // This is a workaround for a parsing bug that sometimes
        // crops up with connect commands.  The parser will think
        // it is done before it reads the last newline of some
        // properly formatted connect requests
        if(m_http->isDone() && (bytesRead < len)) {
            if(m_http->isConnect() && ((len-bytesRead) == 1) && (buffer[bytesRead] == '\n')) {
                break;
            } else {
                assert(false);
            }
        }
    }
}

string HTTPRequest::getHost()
{
    return m_http->getHost();
}
string HTTPRequest::getRequest()
{
    return m_http->getProxyRequest();
}
string HTTPRequest::getUrl()
{
    return m_http->getUrl();
}

bool HTTPRequest::isConnect()
{
    return m_http->isConnect();
}
