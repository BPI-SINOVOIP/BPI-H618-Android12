/*
 * Copyright (C) 2017-2018 NXP Semiconductors
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

#include "NCIParser.h"

#include <iostream>

using namespace std;

#define C_TO_CPP(c2cpp) (reinterpret_cast<NCI_Parser*>(c2cpp))
#define CPP_TO_C(cpp2c) (reinterpret_cast<void*>(cpp2c))

NCI_Parser_Interface::NCI_Parser_Interface() {
  // TODO
}

NCI_Parser_Interface::~NCI_Parser_Interface() {
  // TODO
}

void* native_createParser() { return CPP_TO_C(NCI_Parser::getInstance()); }

void native_destroyParser(void* psNNP) { C_TO_CPP(psNNP)->resetInstance(); }

void native_initParser(void* psNNP) { C_TO_CPP(psNNP)->initParser(); }

void native_deinitParser(void* psNNP) { C_TO_CPP(psNNP)->deinitParser(); }

void native_parseNciMsg(void* psNNP, unsigned char* msg, unsigned short len) {
  C_TO_CPP(psNNP)->parseNciPacket(msg, len);
}
