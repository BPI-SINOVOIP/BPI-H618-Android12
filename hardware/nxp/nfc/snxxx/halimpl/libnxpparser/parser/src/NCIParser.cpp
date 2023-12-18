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
#include "NCILxDebugDecoder.h"
#include "phOsal_Posix.h"

#include <stdlib.h>
#include <cstring>

using namespace std;

NCI_Parser* NCI_Parser::mpNciParser = nullptr;

NCI_Parser::NCI_Parser() {
  mTaskRunning = false;
  if (mpNciPropDecoder == nullptr) mpNciPropDecoder = new NCI_Decoder_Prop();
  // mpNciStandardDecoder = new NCI_Decoder_Standard();
}

NCI_Parser::~NCI_Parser() {
  mpNciParser = nullptr;
  delete mpNciPropDecoder;
  // delete mpNciStandardDecoder;
}

NCI_Parser* NCI_Parser::getInstance() {
  if (mpNciParser == nullptr) {
    mpNciParser = new NCI_Parser();
    return (mpNciParser);
  } else
    return (mpNciParser);
}

void NCI_Parser::resetInstance() {
  if (mpNciParser != nullptr) delete mpNciParser;
}

void* parsingTask(__attribute__((unused)) void* pvParams) {
  while (1) {
    if (!NCI_Parser::getInstance()->mTaskRunning) {
      phOsal_LogDebug(
          (const unsigned char*)"<<<<<<<<<<Stopping Parser Task>>>>>>>>>>");
      break;
    }
    phOsal_LogDebug(
        (const unsigned char*)"<<<<<<<<<<Running Parser Task>>>>>>>>>>");
    NCI_Parser::getInstance()->decodeNciPacket(phOsalAdapt_GetData());
  }
  return nullptr;
}

void NCI_Parser::initParser() {
  void* pvParserContext = nullptr;
  ADAPTSTATUS dwAdaptStatus = ADAPTSTATUS_FAILED;

  dwAdaptStatus = phOsalAdapt_InitOsal(pvParserContext);
  if (dwAdaptStatus == ADAPTSTATUS_SUCCESS) {
    mTaskRunning = true;
    phOsalAdapt_StartTask((void*)parsingTask, this);
  }
}

void NCI_Parser::deinitParser() {
  void* pvTaskHandle = nullptr;
  ADAPTSTATUS dwAdaptStatus = ADAPTSTATUS_FAILED;

  mTaskRunning = false;
  dwAdaptStatus = phOsalAdapt_StopTask(pvTaskHandle);
  if (dwAdaptStatus == ADAPTSTATUS_SUCCESS) {
    phOsalAdapt_DeinitOsal();
  }
}

void NCI_Parser::parseNciPacket(unsigned char* pMsg, unsigned short len) {
  sQueueData_t* psQueueData = nullptr;

  if (pMsg != nullptr) {
    psQueueData = (sQueueData_t*)malloc(sizeof(sQueueData_t));
    memset(psQueueData, 0, sizeof(sQueueData_t));
    memcpy(psQueueData->buffer, pMsg, len);
    psQueueData->len = len;
  }

  phOsalAdapt_SendData(psQueueData);
}

void NCI_Parser::decodeNciPacket(psQueueData_t nciPacket) {
  if (mpNciPropDecoder != nullptr) {
    mpNciPropDecoder->getLxDebugDecoder().processLxDbgNciPkt(nciPacket->buffer,
                                                             nciPacket->len);
    free(nciPacket);
  }
}
