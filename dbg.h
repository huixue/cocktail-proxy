#ifndef _DBG_H_
#define _DBG_H_

#include <cstdio>

#define cache_dbg(...);

#define httpreq_dbg(...);
    
#define mitm_dbg(...);

#define ui_dbg(...);

//#define cache_dbg(...) do{printf(__VA_ARGS__);}while(0);
    
#define httpreq_dbg(...) do{printf(__VA_ARGS__);}while(0);

//#define mitm_dbg(...) do{printf(__VA_ARGS__);}while(0);

#define ui_dbg(...) do{printf(__VA_ARGS__);}while(0);

#endif
