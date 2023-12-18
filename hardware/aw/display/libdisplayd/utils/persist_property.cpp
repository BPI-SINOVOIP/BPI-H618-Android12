
/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <cutils/properties.h>
#include "persist_property.h"

#include "debug.h"

PersistProperty::PersistProperty(const char *path)
{
    memset(mPath, 0, 64);
    strncpy(mPath, path, 64 - 1);
}

PersistProperty::~PersistProperty()
{}

int PersistProperty::write(const char *value)
{
    if (property_set(mPath, value) != 0) {
        dd_error("property_set error: %s --> %s", mPath, value);
        return -1;
    }
    return 0;
}

int PersistProperty::read(char *buf, size_t size)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(mPath, value, "");
    strncpy(buf, value, size-1);
    buf[size-1] = '\0';
    return 0;
}

