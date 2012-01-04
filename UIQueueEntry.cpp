#include "UIQueueEntry.h"
#include "UI.h"

UIQueueEntry::UIQueueEntry(string element, int port, int /*seq_num*/) {
    m_element = element;
        //make everyone access it once
    bitmap[0] = bitmap[1] = bitmap[2] = 1;
    m_port = port;
}

bool UIQueueEntry::alreadyGot(int port) {
    int browserId = port - UIMgr::REP_1;
    if(bitmap[browserId] <= 0)
        return true;
    return false;
}

bool UIQueueEntry::allGot() {
    if(bitmap[0] <= 0 && bitmap[1] <= 0)
        return true;
    return false;
}

void UIQueueEntry::changeBitMap(int port) {
    int browserId = port - UIMgr::REP_1;
    bitmap[browserId]--;
}

string UIQueueEntry::getElementString() {
    return m_element;
}

int UIQueueEntry::getPort() {
    return m_port;
}

