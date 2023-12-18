/*
 * Copyright (C) 2012-2018 NXP Semiconductors
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

/*
 * NXP ESE features macros definitions
 */

#ifndef NXP_ESE_FEATURES_H
#define NXP_ESE_FEATURES_H

#if (NFC_NXP_CHIP_TYPE == PN65T)
#define NXP_POWER_SCHEME_SUPPORT FALSE
#define NXP_ESE_P73_ISO_RST FALSE
#define NXP_ESE_END_OF_SESSION FALSE
#define NXP_NFCC_SPI_FW_DOWNLOAD_SYNC FALSE
#define NXP_ESE_JCOP_DWNLD_PROTECTION FALSE
#define NXP_SECURE_TIMER_SESSION FALSE
#elif (NFC_NXP_CHIP_TYPE == PN66T)
#define NXP_POWER_SCHEME_SUPPORT FALSE
#define NXP_ESE_P73_ISO_RST FALSE
#define NXP_ESE_END_OF_SESSION FALSE
#define NXP_NFCC_SPI_FW_DOWNLOAD_SYNC FALSE
#define NXP_ESE_JCOP_DWNLD_PROTECTION FALSE
#define NXP_SECURE_TIMER_SESSION FALSE
#elif (NFC_NXP_CHIP_TYPE == PN67T)
#define NXP_POWER_SCHEME_SUPPORT FALSE
#define NXP_ESE_P73_ISO_RST FALSE
#define NXP_ESE_END_OF_SESSION FALSE
#define NXP_NFCC_SPI_FW_DOWNLOAD_SYNC FALSE
#define NXP_ESE_JCOP_DWNLD_PROTECTION FALSE
#define NXP_SECURE_TIMER_SESSION FALSE
#elif (NFC_NXP_CHIP_TYPE == PN80T)
#define NXP_POWER_SCHEME_SUPPORT TRUE
#define NXP_ESE_P73_ISO_RST TRUE
#define NXP_ESE_END_OF_SESSION TRUE
#define NXP_ESE_WTX_RES_DELAY TRUE
#define NXP_ESE_JCOP_DWNLD_PROTECTION TRUE
#define NXP_NFCC_SPI_FW_DOWNLOAD_SYNC TRUE
#define NXP_SECURE_TIMER_SESSION TRUE
#elif (NFC_NXP_CHIP_TYPE == PN81A)
#define NXP_POWER_SCHEME_SUPPORT TRUE
#define NXP_ESE_P73_ISO_RST TRUE
#define NXP_ESE_END_OF_SESSION TRUE
#define NXP_ESE_WTX_RES_DELAY TRUE
#define NXP_ESE_JCOP_DWNLD_PROTECTION TRUE
#define NXP_NFCC_SPI_FW_DOWNLOAD_SYNC TRUE
#define NXP_SECURE_TIMER_SESSION TRUE
#endif

#endif /* end of #ifndef NXP_ESE_FEATURES_H */
