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

#ifndef NCILXDEBUGDECODER_H_
#define NCILXDEBUGDECODER_H_

#include "phOsal_Adaptation.h"

#include <string>

using namespace std;

#ifdef __cplusplus
extern "C" { /* Assume C declarations for C++ */
#endif       /* __cplusplus */

#define MAX_TLV 15

/*
typedef struct timeStamp {
    uint16_t timeStampMs;                     //millisec elapsed after last RF
On Event. Tells Raw RSSI values in case of RSSI debug mode uint16_t timeStampUs;
//microsec elapsed after last RF On Event. Insignificant values in case of RSSI
debug mode } stimeStamp_t, *pstimeStamp_t;

typedef struct cliffState {
    uint8_t  cliffStateTriggerType;           //L1 or L2 or Felica Event Name
    uint8_t  cliffStateTriggerTypeDirection;  //Either Tx or Rx
    uint8_t  cliffStateRFTechNMode;           //CLIFF RF Technology and Mode
} scliffState_t, *pscliffState_t;

typedef struct rssiValues {
    uint8_t  rawRSSIADC;                      //When RSSI debug mode is enabled
    uint8_t  rawRSSIAGC;                      //When RSSI debug mode is enabled
    uint16_t intrpltdRSSI;                    //When RSSI debug mode is enabled
} srssiValues_t, *psrssiValues_t;

typedef struct apcValues {
    uint16_t APC;                             //In case of Tx event
    uint8_t  residualCarrier;
    uint8_t  numDriver;
    float          vtxAmp;
    float          vtxLDO;
    uint16_t txVpp;
} sapcValues_t, *psapcValues_t;

typedef struct felicaInfo {
    string         felicaCmdCode;
    string         felicaSysCode;
    string         felicaRspCode;
    uint8_t  felicaRspCodeStatusFlags[2];
    string         felicaMisc;
    string         eddFelica;
} sfelicaInfo_t, *psfelicaInfo_t;

typedef struct extraDbgData {
    uint8_t  eddL1Error;
    uint8_t  eddL1RxNak;
    uint8_t  eddL1TxErr;
    uint16_t eddL178164RetCode;
    uint8_t  eddL2WUP;
    uint8_t  eddFelica;
} sextraDbgData_t, *psextraDbgData_t;
*/

typedef struct {
  uint16_t timeStampMs;  // millisec elapsed after last RF On Event. Tells Raw
                         // RSSI values in case of RSSI debug mode
  uint16_t timeStampUs;  // microsec elapsed after last RF On Event.
                         // Insignificant values in case of RSSI debug mode
  uint8_t* pCliffStateTriggerType;           // L1 or L2 or Felica Event Name
  uint8_t* pCliffStateTriggerTypeDirection;  // Either Tx or Rx
  uint8_t* pCliffStateRFTechNMode;           // CLIFF RF Technology and Mode
  uint8_t rawRSSIADC;                        // When RSSI debug mode is enabled
  uint8_t rawRSSIAGC;                        // When RSSI debug mode is enabled
  uint8_t intrpltdRSSI[2];                   // When RSSI debug mode is enabled
  uint8_t APC[2];                            // In case of Tx event
  uint8_t* pEddL1Error;
  uint8_t* pEddL1RxNak;
  uint8_t* pEddL1TxErr;
  uint8_t eddL178164RetCode[2];
  uint8_t* pEddL2WUP;
  uint8_t felicaCmdCode;
  uint8_t felicaSysCode[2];
  uint8_t felicaRspCode;
  uint8_t felicaRspCodeStatusFlags[2];
  uint8_t* pFelicaMisc;
  uint8_t eddFelica;
  uint8_t residualCarrier;
  uint8_t numDriver;
  int16_t vtxAmp;
  float vtxLDO;
  uint16_t txVpp;
} sDecodedInfo_t, *psDecodedInfo_t;

typedef struct {
  uint8_t* pLxNtf;
  uint16_t LxNtfLen;
} sLxNtfCoded_t, *psLxNtfCoded_t;

typedef struct {
  uint8_t baseIndex;
  uint8_t milliSecOffset;
  uint8_t microSecOffset;  // Not significant in case of RSSI debug mode
  uint8_t rawRSSIOffset;   // In case of RSSI debug enabled
  uint8_t intrpltdRSSIOffset;
  uint8_t apcOffset;
  uint8_t cliffStateTriggerTypeOffset;
  uint8_t cliffStateRFTechModeOffset;
  uint8_t retCode78164Offset;
  uint8_t felicaCmdOffset;
  uint8_t felicaSysCodeOffset;
  uint8_t felicaRspCodeOffset;
  uint8_t felicaRspStatusFlagsOffset;
  uint8_t felicaMiscOffset;
  uint8_t eddOffset;
  uint8_t eddFelicaOffset;
} sLxNtfDecodingInfo_t, *psLxNtfDecodingInfo_t;

typedef struct {
  sDecodedInfo_t sInfo;
} sL1NtfDecoded_t, *psL1NtfDecoded_t;

typedef struct {
  uint8_t tlvCount;
  sDecodedInfo_t sTlvInfo[MAX_TLV];
} sL2NtfDecoded_t, *psL2NtfDecoded_t;

typedef struct {
  uint8_t level;
  psL1NtfDecoded_t psL1NtfDecoded;
  psL2NtfDecoded_t psL2NtfDecoded;
} sLxNtfDecoded_t, *psLxNtfDecoded_t;

class NCI_LxDebug_Decoder {
 private:
  static NCI_LxDebug_Decoder* mLxDbgDecoder;

  uint8_t mL2DebugMode;        // bit:0 Byte0
  uint8_t mFelicaRFDebugMode;  // bit:1 Byte0
  uint8_t mFelicaSCDebugMode;  // bit:2 Byte0
  uint8_t mL1DebugMode;        // bit:4 Byte0
  uint8_t m7816DebugMode;      // bit:6 Byte0
  uint8_t mRssiDebugMode;      // bit:0 Byte1

  float mLOOKUP_VTXLDO[5] = {3.0, 3.3, 3.6, 4.5, 4.7};    // in Volts
  int16_t mLOOKUP_VTXAMP[4] = {-150, -250, -500, -1000};  // in mVolts
  uint8_t mLOOKUP_NUMDRIVER[2] = {1, 2};
  uint8_t mLOOKUP_RESCARRIER[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                    22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

  uint16_t mLOOKUP_VTXLDO_BITMASK = 0x0700;
  uint16_t mLOOKUP_VTXAMP_BITMASK = 0x00C0;
  uint16_t mLOOKUP_NUMDRIVER_BITMASK = 0x0020;
  uint16_t mLOOKUP_RESCARRIER_BITMASK = 0x001F;

  uint8_t mCLF_EVT_DIRECTION[2][11] = {"CLF_EVT_RX", "CLF_EVT_TX"};

  uint8_t mCLF_STAT_L1_TRIG_TYPE[15][27] = {
      "CLF_L1_EVT_RFU",         "CLF_L1_EVT_ACTIVATED",
      "CLF_L1_EVT_DATA_RX",     "CLF_L1_EVT_RX_DESELECT",
      "CLF_L1_EVT_RX_WTX",      "CLF_L1_EVT_ERROR",
      "CLF_L1_EVT_RX_ACK",      "CLF_L1_EVT_RX_NACK",
      "CLF_L1_EVT_DATA_TX",     "CLF_L1_EVT_WTX_AND_DATA_TX",
      "CLF_L1_EVT_TX_DESELECT", "CLF_L1_EVT_TX_WTX",
      "CLF_L1_EVT_TX_ACK",      "CLF_L1_EVT_TX_NAK",
      "CLF_L1_EVT_EXTENDED"};

  uint8_t mCLF_STAT_L2_TRIG_TYPE[12][31] = {
      "CLF_L2_EVT_RFU", "CLF_L2_EVT_MODULATION_DETECTED", "CLF_L2_EVT_DATA_RX",
      "CLF_L2_EVT_TIMEOUT",
      "CLF_L2_EVT_ACTIVE_ISO14443_3",  // Internal to card Layer3 activation
      "CLF_L2_EVT_ERROR", "CLF_L2_EVT_SENSING",
      "CLF_L2_EVT_ACTIVE_ISO14443_4",  // APC, Because Layer4 activation sent to
                                       // reader as Tx
      "CLF_L2_EVT_RFON", "CLF_L2_EVT_RFOFF", "CLF_L2_EVT_DATA_TX",
      "CLF_L2_EVT_WUP_IOT_RECONFIG"  // APC
  };

  uint8_t mCLF_STAT_RF_TECH_MODE[14][42] = {
      "CLF_STATE_TECH_RFU",
      "CLF_STATE_TECH_CE_A",
      "CLF_STATE_TECH_CE_B",
      "CLF_STATE_TECH_CE_F",
      "CLF_STATE_TECH_NFCIP1_TARGET_PASSIVE_A",
      "CLF_STATE_TECH_NFCIP1_TARGET_PASSIVE_F",
      "CLF_STATE_TECH_NFCIP1_TARGET_ACTIVE_A",
      "CLF_STATE_TECH_NFCIP1_TARGET_ACTIVE_F",
      "CLF_STATE_TECH_RM_A",
      "CLF_STATE_TECH_RM_B",
      "CLF_STATE_TECH_RM_F",
      "CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_A",
      "CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_B",
      "CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_F"};

  uint8_t mEDD_L1_ERROR[8][34] = {
      "L1_ERROR_EDD_RF_TIMEOUT",           "L1_ERROR_EDD_RF_CRC_ERROR",
      "L1_ERROR_EDD_RF_COLLISION",         "L1_ERROR_EDD_RX_DATA_OVERFLOW",
      "L1_ERROR_EDD_RX_PROTOCOL_ERROR",    "L1_ERROR_EDD_TX_NO_DATA_ERROR",
      "L1_ERROR_EDD_EXTERNAL_FIELD_ERROR", "L1_ERROR_EDD_RXDATA_LENGTH_ERROR"};

  uint8_t mEDD_L1_TX_ERROR[28] = "L1_TX_EVT_EDD_DPLL_UNLOCKED";

  uint8_t mEDD_L1_RX_NAK[5][26] = {
      "L1_RX_NACK_EDD_IOT_STAGE1", "L1_RX_NACK_EDD_IOT_STAGE2",
      "L1_RX_NACK_EDD_IOT_STAGE3", "L1_RX_NACK_EDD_IOT_STAGE4",
      "L1_RX_NACK_EDD_IOT_STAGE5"};

  uint8_t mEDD_L2_WUP[5][22] = {
      "L2_EDD_WUP_IOT_STAGE1", "L2_EDD_WUP_IOT_STAGE2", "L2_EDD_WUP_IOT_STAGE3",
      "L2_EDD_WUP_IOT_STAGE4", "L2_EDD_WUP_IOT_STAGE5"};

  uint8_t mFELICA_MISC_EVT[5][34] = {
      "FLC_MISC_EVT_RFU", "FLC_MISC_EVT_GENERIC_ERROR",
      "FLC_MISC_EVT_EMPTY_FRAME_FROM_ESE", "FLC_MISC_EVT_BUFFER_OVERFLOW",
      "FLC_MISC_EVT_RF_ERROR"};

  typedef enum {
    CLF_L1_EVT_RFU = 0x00,
    CLF_L1_EVT_ACTIVATED = 0x01,  // APC
    CLF_L1_EVT_DATA_RX = 0x02,
    CLF_L1_EVT_RX_DESLECT = 0x03,
    CLF_L1_EVT_RX_WTX = 0x04,
    CLF_L1_EVT_ERROR = 0x05,
    CLF_L1_EVT_RX_ACK = 0x06,
    CLF_L1_EVT_RX_NACK = 0x07,
    CLF_L1_EVT_DATA_TX = 0x08,          // APC
    CLF_L1_EVT_WTX_AND_DATA_TX = 0x09,  // APC
    CLF_L1_EVT_TX_DESELECT = 0x0A,      // APC
    CLF_L1_EVT_TX_WTX = 0x0B,           // APC
    CLF_L1_EVT_TX_ACK = 0x0C,           // APC
    CLF_L1_EVT_TX_NAK = 0x0D,           // APC
    CLF_L1_EVT_EXTENDED = 0x0E          // APC
  } CliffStateL1EventType_t;

  typedef enum {
    CLF_L2_EVT_RFU = 0x00,
    CLF_L2_EVT_MODULATION_DETECTED = 0x01,
    CLF_L2_EVT_DATA_RX = 0x02,
    CLF_L2_EVT_TIMEOUT = 0x03,
    CLF_L2_EVT_ACTIVE_ISO14443_3 = 0x04,  // Internal to card Layer3 activation
    CLF_L2_EVT_ERROR = 0x05,
    CLF_L2_EVT_SENSING = 0x06,
    CLF_L2_EVT_ACTIVE_ISO14443_4 =
        0x07,  // APC, Because Layer4 activation sent to reader as Tx
    CLF_L2_EVT_RFON = 0x08,
    CLF_L2_EVT_RFOFF = 0x09,
    CLF_L2_EVT_DATA_TX = 0x0A,  // APC
    CLF_L2_EVT_WUP_IOT_RECONFIG = 0x0B
  } CliffStateL2EventType_t;

  typedef enum {
    SYSTEM_DEBUG_STATE_L1_MESSAGE =
        0x35,  // RF Exchanges & Events after Activation of NFC-DEP/ISO-DEP
    SYSTEM_DEBUG_STATE_L2_MESSAGE =
        0x36  // RF Exchanges & Events before Activation
  } LxDebugNtfType_t;

  typedef enum {
    L1_EVT_LEN = 0x07,
    L1_EVT_EXTRA_DBG_LEN = 0x08,
    L1_EVT_7816_RET_CODE_LEN = 0x0A
  } L1DebugNtfLen_t;

  typedef enum {
    L2_EVT_TAG_ID = 0x10,
    L2_EVT_FELICA_CMD_TAG_ID = 0x20,
    L2_EVT_FELICA_SYS_CODE_TAG_ID = 0x30,
    L2_EVT_FELICA_RSP_CODE_TAG_ID = 0x40,
    L2_EVT_FELICA_MISC_TAG_ID = 0x50
  } L2DebugNtfTLVTagId_t;

  typedef enum {
    L2_EVT_TAG_ID_LEN = 0x07,
    L2_EVT_TAG_ID_EXTRA_DBG_LEN = 0x08,
    L2_EVT_FELICA_CMD_TAG_ID_LEN = 0x07,
    L2_EVT_FELICA_CMD_TAG_ID_EXTRA_DBG_LEN = 0x08,
    L2_EVT_FELICA_SYS_CODE_TAG_ID_LEN = 0x06,
    L2_EVT_FELICA_RSP_CODE_TAG_ID_LEN = 0x09,
    L2_EVT_FELICA_RSP_CODE_TAG_ID_EXTRA_DBG_LEN = 0x0A,
    L2_EVT_FELICA_MISC_TAG_ID_LEN = 0x05,
    L2_EVT_FELICA_MISC_TAG_ID_EXTRA_DBG_LEN = 0x06
  } L2DebugNtfTLVLength_t;

  typedef enum {
    FLC_RM_EVT_RFU = 0x00,
    FLC_RM_EVT_ACTIVATED = 0x01,
    FLC_RM_EVT_DATA_RX,
    FLC_RM_EVT_RX_READ,
    FLC_RM_EVT_RX_WRITE,
    FLC_RM_EVT_ERROR,
    FLC_RM_EVT_DATA_TX,
    FLC_RM_EVT_TX_READ,
    FLC_RM_EVT_TX_WRITE
  } FelicaRMEvents_t;

  typedef enum {
    CLF_STATE_TECH_RFU = 0x00,
    CLF_STATE_TECH_CE_A = 0x10,
    CLF_STATE_TECH_CE_B = 0x20,
    CLF_STATE_TECH_CE_F = 0x30,
    CLF_STATE_TECH_NFCIP1_TARGET_PASSIVE_A = 0x40,
    CLF_STATE_TECH_NFCIP1_TARGET_PASSIVE_F = 0x50,
    CLF_STATE_TECH_NFCIP1_TARGET_ACTIVE_A = 0x60,
    CLF_STATE_TECH_NFCIP1_TARGET_ACTIVE_F = 0x70,
    CLF_STATE_TECH_RM_A = 0x80,
    CLF_STATE_TECH_RM_B = 0x90,
    CLF_STATE_TECH_RM_F = 0xA0,
    CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_A = 0xC0,
    CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_B = 0xD0,
    CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_F = 0xE0
  } CliffStateTechType_t;

  typedef enum {
    L1_ERROR_EDD_RF_TIMEOUT = 0x01,
    L1_ERROR_EDD_RF_CRC_ERROR = 0x02,
    L1_ERROR_EDD_RF_COLLISION = 0x04,
    L1_ERROR_EDD_RX_DATA_OVERFLOW = 0x05,
    L1_ERROR_EDD_RX_PROTOCOL_ERROR = 0x06,
    L1_ERROR_EDD_TX_NO_DATA_ERROR = 0x07,
    L1_ERROR_EDD_EXTERNAL_FIELD_ERROR = 0x0A,
    L1_ERROR_EDD_RXDATA_LENGTH_ERROR = 0x80
  } ExtraDebugDataforL1_ERROR_t;

  typedef enum {
    L1_TX_EVT_EDD_DPLL_UNLOCKED = 0x84
  } ExtraDebugData_L1_ALL_TX_EVENTS_t;

  typedef enum {
    L1_RX_NACK_EDD_IOT_STAGE1 = 0x85,
    L1_RX_NACK_EDD_IOT_STAGE2 = 0x86,
    L1_RX_NACK_EDD_IOT_STAGE3 = 0x87,
    L1_RX_NACK_EDD_IOT_STAGE4 = 0x88,
    L1_RX_NACK_EDD_IOT_STAGE5 = 0x89
  } ExtraDebugData_L1_RX_NACK_t;

  typedef enum {
    L2_EDD_WUP_IOT_STAGE1 = 0x01,
    L2_EDD_WUP_IOT_STAGE2 = 0x02,
    L2_EDD_WUP_IOT_STAGE3 = 0x03,
    L2_EDD_WUP_IOT_STAGE4 = 0x04,
    L2_EDD_WUP_IOT_STAGE5 = 0x05,
  } ExtraDebugData_CLF_L2_EVT_WUP_IOT_RECONFIG_t;

  typedef enum {
    L2_EDD_FLC_CMD_CODE = 0x01,
    L2_EDD_FLC_RSP_CODE = 0x02,
    L2_EDD_FLC_MISC = 0x03
  } ExtraDebugData_Felica_t;

  typedef enum {
    FLC_MISC_EVT_RFU = 0x00,
    FLC_MISC_EVT_GENERIC_ERROR = 0x01,
    FLC_MISC_EVT_EMPTY_FRAME_FROM_ESE,
    FLC_MISC_EVT_BUFFER_OVERFLOW,
    FLC_MISC_EVT_RF_ERROR = 0x05
  } FelicaMiscEventType_t;

  NCI_LxDebug_Decoder();
  void setLxDebugModes(uint8_t* pNciPkt, uint16_t pktLen);
  void parseL1DbgNtf(psLxNtfCoded_t psLxNtfCoded,
                     psLxNtfDecoded_t psLxNtfDecoded);
  void parseL2DbgNtf(psLxNtfCoded_t psLxNtfCoded,
                     psLxNtfDecoded_t psLxNtfDecoded);
  void decodeTimeStamp(psLxNtfCoded_t psLxNtfCoded,
                       psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                       psLxNtfDecoded_t psLxNtfDecoded);
  void decodeAPCTable(psLxNtfCoded_t psLxNtfCoded,
                      psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                      psLxNtfDecoded_t psLxNtfDecoded);
  void decodeRSSIValues(psLxNtfCoded_t psLxNtfCoded,
                        psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                        psLxNtfDecoded_t psLxNtfDecoded);
  void decodeCLIFFState(psLxNtfCoded_t psLxNtfCoded,
                        psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                        psLxNtfDecoded_t psLxNtfDecoded);
  void decodeRFTechMode(psLxNtfCoded_t psLxNtfCoded,
                        psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                        psLxNtfDecoded_t psLxNtfDecoded);
  void decodeTriggerType(psLxNtfCoded_t psLxNtfCoded,
                         psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                         psLxNtfDecoded_t psLxNtfDecoded);
  void decode78164RetCode(psLxNtfCoded_t psLxNtfCoded,
                          psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                          psLxNtfDecoded_t psLxNtfDecoded);
  void decodeFelicaCmdCode(psLxNtfCoded_t psLxNtfCoded,
                           psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                           psLxNtfDecoded_t psLxNtfDecoded);
  void decodeFelicaSystemCode(psLxNtfCoded_t psLxNtfCoded,
                              psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                              psLxNtfDecoded_t psLxNtfDecoded);
  void decodeFelicaRspCode(psLxNtfCoded_t psLxNtfCoded,
                           psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                           psLxNtfDecoded_t psLxNtfDecoded);
  void decodeFelicaMiscCode(psLxNtfCoded_t psLxNtfCoded,
                            psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                            psLxNtfDecoded_t psLxNtfDecoded);
  void decodeExtraDbgData(psLxNtfCoded_t psLxNtfCoded,
                          psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
                          psLxNtfDecoded_t psLxNtfDecoded);
  void calculateTxVpp(psLxNtfDecoded_t psLxNtfDecoded);
  void printLxDebugInfo(psLxNtfDecoded_t psLxNtfDecoded);

 public:
  ~NCI_LxDebug_Decoder();
  static NCI_LxDebug_Decoder& getInstance();
  void processLxDbgNciPkt(uint8_t* pNciPkt, uint16_t pktLen);
};
#ifdef __cplusplus
} /* Assume C declarations for C++ */
#endif /* __cplusplus */

#endif /* NCILXDEBUGDECODER_H_ */
