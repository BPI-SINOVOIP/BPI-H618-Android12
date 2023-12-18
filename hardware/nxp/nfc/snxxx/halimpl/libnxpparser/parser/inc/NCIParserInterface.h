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

#ifndef NCIPARSERINTERFACE_H_
#define NCIPARSERINTERFACE_H_

#ifdef __cplusplus
extern "C" { /* Assume C declarations for C++ */
#endif       /* __cplusplus */
class NCI_Parser_Interface {
 public:
  NCI_Parser_Interface();
  virtual ~NCI_Parser_Interface();
  virtual void initParser() = 0;
  virtual void deinitParser() = 0;
  virtual void parseNciPacket(unsigned char* msg, unsigned short len) = 0;
};

void* native_createParser();
void native_destroyParser(void* psNNP);
void native_initParser(void* psNNP);
void native_deinitParser(void* psNNP);
void native_parseNciMsg(void* psNNP, unsigned char* msg, unsigned short len);

#ifdef __cplusplus
} /* Assume C declarations for C++ */
#endif /* __cplusplus */

#endif /* NCIPARSERINTERFACE_H_ */
