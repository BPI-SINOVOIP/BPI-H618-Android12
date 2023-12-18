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

#ifndef NCIPARSER_H_
#define NCIPARSER_H_

#include "NCIDecoderProp.h"
#include "NCIDecoderStandard.h"
#include "NCIParserInterface.h"
#include "phOsal_Adaptation.h"

#ifdef __cplusplus
extern "C" { /* Assume C declarations for C++ */
#endif       /* __cplusplus */
class NCI_Parser : public NCI_Parser_Interface {
 private:
  NCI_Parser();
  volatile bool mTaskRunning;
  static NCI_Parser* mpNciParser;
  NCI_Decoder_Prop* mpNciPropDecoder = nullptr;
  // NCI_Decoder_Standard *mpNciStandardDecoder;
  void decodeNciPacket(psQueueData_t nciPacket);

 public:
  ~NCI_Parser();
  void initParser();
  void deinitParser();
  static NCI_Parser* getInstance();
  static void resetInstance();
  friend void* parsingTask(void*);
  void parseNciPacket(unsigned char* pMsg, unsigned short len);
};
#ifdef __cplusplus
} /* Assume C declarations for C++ */
#endif /* __cplusplus */

#endif /* NCIPARSER_H_ */
