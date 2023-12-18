/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>

#include "EnhanceBase.h"

using namespace sunxi;

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [-o] [-d] [-x]\n", argv[0]);
        return 0;
    }

    std::unique_ptr<EnhanceBase> enhanceHandle = createEnhanceHandle();
    enhanceHandle->setup();

    if (!strcmp(argv[1], "-o")) {
        int ret = enhanceHandle->setSmartBackLight(EnhanceBase::kSmartBackLightOn);
        printf("open smart backlight return %d\n", ret);
    } else if (!strcmp(argv[1], "-d")) {
        int ret = enhanceHandle->setSmartBackLight(EnhanceBase::kSmartBackLightDemo);
        printf("open smart backlight Demo mode return %d\n", ret);
    } else {
        int ret = enhanceHandle->setSmartBackLight(EnhanceBase::kSmartBackLightOff);
        printf("close smart backlight return %d\n", ret);
    }
    return 0;
}
