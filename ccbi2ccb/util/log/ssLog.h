#ifndef __SSLOG_H_
#define __SSLOG_H_

#include "assert.h"

static const int kMaxLogLen = 16 * 1024;

/**
@brief Output Debug message
*/
void SSLog(const char * pszFormat, ...);


#define ASSERT_FAIL_UNEXPECTED_PROPERTYTYPE(PROPERTYTYPE) SSLog("Unexpected property type: '%d'!\n", PROPERTYTYPE); assert(false)

#endif