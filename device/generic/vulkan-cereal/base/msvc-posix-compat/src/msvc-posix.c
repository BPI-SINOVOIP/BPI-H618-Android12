// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "msvc-posix.h"

// TODO(joshuaduong): Support unicode (b/117322783)
//#include "android/base/system/Win32UnicodeString.h"

#include <io.h>

#include <stdio.h>
#include <stdlib.h>

int mkstemp(char* t) {
    // TODO(joshuaduong): Support unicode (b/117322783)
    int len = strlen(t) + 1;
    errno_t err = _mktemp_s(t, len);

    if (err != 0) {
        return -1;
    }

    return _sopen(t, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYRW,
                  _S_IREAD | _S_IWRITE);
}

// From https://msdn.microsoft.com/en-us/library/28d5ce15.aspx
int asprintf(char** buf, const char* format, ...) {
    va_list args;
    int len;

    if (buf == NULL) {
        return -1;
    }

    // retrieve the variable arguments
    va_start(args, format);

    len = _vscprintf(format, args)  // _vscprintf doesn't count
          + 1;                      // terminating '\0'

    if (len <= 0) {
        return len;
    }

    *buf = (char*)malloc(len * sizeof(char));

    vsprintf(*buf, format, args);  // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
    return len;
}

// From https://msdn.microsoft.com/en-us/library/28d5ce15.aspx
static int vasprintf(char** buf, const char* format, va_list args) {
    int len;

    if (buf == NULL) {
        return -1;
    }

    len = _vscprintf(format, args)  // _vscprintf doesn't count
          + 1;                      // terminating '\0'

    if (len <= 0) {
        return len;
    }

    *buf = (char*)malloc(len * sizeof(char));

    vsprintf(*buf, format, args);  // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
    return len;
}
