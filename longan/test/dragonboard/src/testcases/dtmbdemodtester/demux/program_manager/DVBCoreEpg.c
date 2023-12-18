
#include <DVBCoreLog.h>
#include <DVBCoreBitReader.h>
#include <DVBCoreMemory.h>
#include <DVBCoreEpg.h>
#include <stdint.h>
#include "DVBCoreParsePAT.h"
#include "DVBCoreParseCAT.h"
#include "DVBCoreParsePMT.h"
#include "DVBCoreParseSDT.h"
#include "DVBCoreParseNIT.h"
#include "DVBCoreParseEIT.h"
#include "DVBCoreParseTDT.h"
#include <pthread.h>

#if !OPEN_CHECK
#undef DVBCORE_CHECK
#define DVBCORE_CHECK(e)
#undef DVBCORE_FORCE_CHECK
#define DVBCORE_FORCE_CHECK(e) \
    do { if(!e){DVBCORE_LOGD("DVBCORE_CHECK(%s) failed.", #e);} } while (0)
#endif

#define MY_LOGV(x, y) \
    do { unsigned tmp = y; /*DVBCORE_LOGV(x, tmp);*/ (void)tmp; } while (0)

DVBCORE_INTERFACE uint16 U16_AT(const uint8 *ptr) 
{
    return ptr[0] << 8 | ptr[1];
}

status_t parseTS(TSParser *mTSParser, DVBCoreBitReaderT *br);
void parseAdaptationField(TSParser *mTSParser, DVBCoreBitReaderT *br, unsigned PID);
status_t parsePID(TSParser *mTSParser, DVBCoreBitReaderT *br, unsigned PID,
	unsigned continuity_counter, unsigned payload_unit_start_indicator);

DVBCORE_INTERFACE status_t PSISectionAppend(PSISection *section, 
	const uint8 *data, uint32 size);
DVBCORE_INTERFACE dvbcore_bool PSISectionIsComplete(PSISection *section);
DVBCORE_INTERFACE dvbcore_bool PSISectionIsEmpty(PSISection *section);
DVBCORE_INTERFACE const uint8 *PSISectionData(PSISection *section);
DVBCORE_INTERFACE int PSISectionSize(PSISection *section);
DVBCORE_INTERFACE void PSISectionClear(PSISection *section);
DVBCORE_INTERFACE void DestroyPSISection(PSISection *section);
DVBCORE_INTERFACE void DestroyPSISections(TSParser *mTSParser);
PSISection *findPSISectionByPID(TSParser *mTSParser, unsigned PID);
static dvbcore_err SetPSISection(TSParser *mTSParser, unsigned PID);

DVBCORE_INTERFACE status_t PSISectionAppend(PSISection *section, const uint8 *data, uint32 size) 
{
    DVBCORE_CHECK(section);
	DVBCoreBufferT *mBuffer = section->mBuffer;
	DVBCoreBufferAppend(mBuffer, data, size);
    return SUCCESS;
}

DVBCORE_INTERFACE dvbcore_bool PSISectionIsComplete(PSISection *section)
{
    DVBCORE_CHECK(section);
	DVBCoreBufferT *mBuffer = section->mBuffer;
	uint32 size;
    if (mBuffer == NULL || (size = DVBCoreBufferGetSize(mBuffer)) < 3) 
	{
        return DVBCORE_FALSE;
    }

    unsigned sectionLength = U16_AT(DVBCoreBufferGetData(mBuffer) + 1) & 0xfff;
    DVBCORE_CHECK((sectionLength & 0xc00) == 0);
    return size >= sectionLength + 3;
}

DVBCORE_INTERFACE dvbcore_bool PSISectionIsEmpty(PSISection *section)
{
    DVBCORE_CHECK(section);
	DVBCoreBufferT *mBuffer = section->mBuffer;
    return mBuffer == NULL || DVBCoreBufferGetSize(mBuffer) == 0;
}

DVBCORE_INTERFACE const uint8 *PSISectionData(PSISection *section)
{
    DVBCORE_CHECK(section);
	DVBCoreBufferT *mBuffer = section->mBuffer;
    return mBuffer == NULL ? NULL : DVBCoreBufferGetData(mBuffer);
}

DVBCORE_INTERFACE int PSISectionSize(PSISection *section)
{
    DVBCORE_CHECK(section);
	DVBCoreBufferT *mBuffer = section->mBuffer;
    return mBuffer == NULL ? 0 : DVBCoreBufferGetSize(mBuffer);
}


DVBCORE_INTERFACE void PSISectionClear(PSISection *section) 
{
    DVBCORE_CHECK(section);
	DVBCoreBufferT *mBuffer = section->mBuffer;
    if (mBuffer != NULL) 
	{
		DVBCoreBufferSetRange(mBuffer, 0, 0);
    }
    section->mPayloadStarted = DVBCORE_FALSE;
}

DVBCORE_INTERFACE void DestroyPSISection(PSISection *section) 
{
    DVBCORE_CHECK(section);
	DVBCoreListDel(&section->node);
	DVBCoreBufferDestroy(section->mBuffer);
	DVBCoreFree(section);
}

DVBCORE_INTERFACE void DestroyPSISections(TSParser *mTSParser) 
{
	PSISection *posPSI, *nextPSI;

    DVBCoreListForEachEntrySafe(posPSI, nextPSI, &mTSParser->mPSISections, node)
    {
        DestroyPSISection(posPSI);
    }

}

status_t feedTSPacket(TSParser *mTSParser, const uint8 *data, int size)
{
	
	DVBCORE_UNUSE(size);
    DVBCORE_CHECK(size == TS_PACKET_SIZE);

    if(mTSParser->bIsEndEitSection == 1)
    {
        return SUCCESS;
    }

	DVBCoreBitReaderT *br = DVBCoreBitReaderCreate(data, TS_PACKET_SIZE);
	status_t err = parseTS(mTSParser, br);
	DVBCoreBitReaderDestroy(br);
    return err;
}

status_t parseTS(TSParser *mTSParser, DVBCoreBitReaderT *br)
{
	int i;
    unsigned sync_byte = DVBCoreBitReaderGetBits(br, 8);
    DVBCORE_CHECK(sync_byte == 0x47u);
	unsigned transport_error_indicator = DVBCoreBitReaderGetBits(br, 1);
	if(transport_error_indicator)
	{
		DVBCORE_LOGE("transport_error_indicator = %u\n", transport_error_indicator);
		return SUCCESS;
	}
	
    unsigned payload_unit_start_indicator = DVBCoreBitReaderGetBits(br, 1);
    MY_LOGV("transport_priority = %u", DVBCoreBitReaderGetBits(br, 1));

    unsigned PID = DVBCoreBitReaderGetBits(br, 13);

	if(mTSParser->enablePid[PID] == 0 || PID != EIT_PID)//only EIT packets can be processed
	{
		//DVBCORE_LOGD("enter %s:%d\n",__FUNCTION__,__LINE__);
		return SUCCESS;
	}
    MY_LOGV("transport_scrambling_control = %u", DVBCoreBitReaderGetBits(br, 2));

    unsigned adaptation_field_control = DVBCoreBitReaderGetBits(br, 2);
    unsigned continuity_counter = DVBCoreBitReaderGetBits(br, 4);

	if (adaptation_field_control == 2 || adaptation_field_control == 3) 
	{
        if(adaptation_field_control == 2)
        {
			DVBCORE_LOGD("enter %s:%d\n",__FUNCTION__,__LINE__);
            return SUCCESS;
        }
        unsigned tmp = DVBCoreBitReaderGetBits(br, 8);
        if(tmp > 182)
        {
			DVBCORE_LOGD("enter %s:%d\n",__FUNCTION__,__LINE__);
            return SUCCESS;
        }
        DVBCoreBitReaderSkipBits(br, tmp << 3);
    }

    status_t err = SUCCESS;

    if (adaptation_field_control == 1 || adaptation_field_control == 3) 
	{
		err = parsePID(mTSParser, 
				br, PID, continuity_counter, payload_unit_start_indicator);
    }
	else
	{
		DVBCORE_LOGD("enter %s:%d\n",__FUNCTION__,__LINE__);
	}
    return err;
}

PSISection *findPSISectionByPID(TSParser *mTSParser, unsigned PID)
{
    PSISection *item = NULL;
    DVBCoreListForEachEntry(item, &mTSParser->mPSISections, node)
	{
        if (item->PID == PID)
        {
            return item;
        }
    }
	return NULL;
}

static unsigned int Crc32Compute(unsigned int initVector, unsigned char* pData, unsigned int dataLen)
{
    static const unsigned int crc32Table[] = 
    {
        0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
        0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
        0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
        0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
        0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
        0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
        0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
        0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
        0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
        0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
        0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
        0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
        0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
        0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
        0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
        0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
        0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
        0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
        0x251d3b9e, 0x21dc2629,	0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
        0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
        0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
        0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
        0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
        0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
        0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
        0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
        0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
        0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
        0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
        0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
        0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
        0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
    };
	
    unsigned int   i;
    unsigned int   j;
    unsigned int   crc;
    
    crc = initVector;

	for (j = 0; j < dataLen;  j++)
	{
		i	= ((crc >> 24) ^ *pData++) & 0xff;
		crc	= (crc << 8) ^ crc32Table[i];
	}
	return crc;
}

static int32 DvbCoreBitReaderSkipBitsSafe(DVBCoreBitReaderT *br, uint32 n)
{
	int32 ret = 0;
	uint32 nLeftNum = 0;
	nLeftNum = DVBCoreBitReaderNumBitsLeft(br);
	if(nLeftNum < n)
	{
		DVBCORE_LOGE("DTMB SKIP BITS SAFE:Skip Bits unSafe\n");
		DVBCoreBitReaderSkipBits(br, nLeftNum);	
		ret = -1;
	}
	else
	{
		DVBCoreBitReaderSkipBits(br, n);
	}
	return ret;
}


status_t parsePSISection(TSParser *mTSParser, PSISection *section)
{
	status_t err = SUCCESS;
	
	DVBCoreBitReaderT *sectionBits = DVBCoreBitReaderCreate(PSISectionData(section),
		PSISectionSize(section));
	
	uint8*	secBufer = (uint8* )PSISectionData(section);
	uint8	secLen = (uint8)((((secBufer[1] & 0x0F) << 8) | secBufer[2]) + 3);
	uint8	table_id = secBufer[0];
	uint8	sec_num  = secBufer[6];	
	uint8	i_num = table_id & 0x0F;
	uint8	segment = sec_num >> 3;
    uint8   last_section_number = secBufer[7];
	uint8	last_segment = secBufer[7] >> 3;
    uint8   last_table_id = 0;
	uint32	service_id = (secBufer[3] << 8) | secBufer[4];
    unsigned int event_cnt = 0;

	if(Crc32Compute(0xffffffff, secBufer, secLen) != 0 
		&& section->PID != EIT_PID
		&& section->PID != SDT_BAT_PID) //for test
	{
		err = SKIP;
		PSISectionClear(section);
		DVBCORE_LOGW("section data may error!\n");
		goto _exit;
	}
	else	
	{
		//DVBCORE_LOGW("section data may ok!\n");
	}
    //DVBCORE_LOGD("parsePSISection section->PID=0x%x", section->PID);

	switch(section->PID)
	{
		case PAT_PID:
			
			err = ParseFromPAT(secBufer, &mTSParser->pProgram, &mTSParser->pat_version_number);
			if(!err)
			{
				//DVBCORE_LOGD("parse PAT successfully!\n");
				pthread_mutex_lock(&mTSParser->mutex); /*lock*/
				pthread_cond_signal(&mTSParser->cond);
				mTSParser->bIsEndPatSection = 1;				
				pthread_mutex_unlock(&mTSParser->mutex); /*unlock*/
			}
			PROGRAM * p = mTSParser->pProgram;			
			while(p)
			{
				if (p->uProgramNumber != 0) 
				{
					if (!findPSISectionByPID(mTSParser, p->uPMT_PID))
					{
						SetPSISection(mTSParser, p->uPMT_PID);
						mTSParser->enablePid[p->uPMT_PID] = 1;
					}
				}
				p = p->next;
			}			
			PSISectionClear(section);
			break;
			
		case NIT_PID:
			
			err = ParseFromNIT(secBufer, &mTSParser->nit_version_number);
			if(!err)
			{
				//DVBCORE_LOGD("parse NIT successfully!\n");
				pthread_mutex_lock(&mTSParser->mutex); /*lock*/
				pthread_cond_signal(&mTSParser->cond);
				mTSParser->bIsEndNitSection = 1;
				pthread_mutex_unlock(&mTSParser->mutex); /*unlock*/
			}
			PSISectionClear(section);
			break;
			
		case EIT_PID:
            if(mTSParser->bIsEndEitSection == 1)
            {
                DVBCORE_LOGD("parse service_id %x eit end ,wait next parse!", mTSParser->prog_num);
                PSISectionClear(section);
			    break;
            }
            //DVBCORE_LOGD("service_id [%d %x], table_id %x, i_num %d, segment: %d ", service_id, service_id, table_id, i_num, segment);

            last_table_id = secBufer[13];

			if((table_id & 0xF0) == 0x50) /* schedule 8 days*/
			{
				if((service_id == mTSParser->prog_num) && !mTSParser->schdl_parse_end[i_num])
				{
                    //DVBCORE_LOGD("service_id [%d %x], i_num %d, segment: %d ", service_id, service_id, i_num, segment);

					if(mTSParser->schdl_count[i_num] == segment)
					{
						err = ParseFromEIT(secBufer, &mTSParser->pProgramEvent[i_num], &mTSParser->schdl_eit_version_number[i_num],
							&mTSParser->schdl_sec_mask[i_num][segment], mTSParser->prog_num, &event_cnt);
						mTSParser->schdl_count[i_num]++;
                        //calc all program events count in this prog_num.
                        mTSParser->event_total_count += event_cnt;

						if(mTSParser->schdl_count[i_num] == (last_segment + 1))
						{
                            mTSParser->schdl_parse_end[i_num] = 1;
                            DVBCORE_LOGD("parse i_num %d schdl end, schdl_count: %d, last_table_id %d !", i_num, mTSParser->schdl_count[i_num], last_table_id);
                            /*judge all eit tables parse complete*/
                            if((table_id == last_table_id && mTSParser->schdl_parse_end[0]) ||
                               (mTSParser->schdl_parse_end[0] && mTSParser->schdl_parse_end[1]))
                            {
                                DVBCORE_LOGD("parser service_id %d schdl finish, total %d !", service_id, mTSParser->event_total_count);
                                mTSParser->bIsEndEitSection = 1;
                            }
						}
						if(err)
						{
							DVBCORE_LOGD("parse EIT failed!\n");
						}
					}
				}
			}
			else if(table_id == 0x4E)/* pf */
			{
            /*schedule tables include p/f table infos,so we not need  to parser p/f table currently*/
#if 0
				if(mTSParser->pf_count == sec_num) 
				{
					if(service_id == mTSParser->prog_num && !mTSParser->pf_parse_end)
					{
						if(sec_num == 0)/* present */
						{
							err = ParseFromEIT(secBufer, &mTSParser->pPresentEvent, &mTSParser->pf_eit_version_number,
								&mTSParser->pf_sec_mask, mTSParser->prog_num, &event_cnt);
							if(err)
							{
								DVBCORE_LOGW("parse EIT failed!\n");
							}
							DVBCORE_LOGD("parse service_id %d EIT present info finish!", service_id);
							mTSParser->pf_count = 1;
                            mTSParser->event_total_count += event_cnt;
						}
						else /* follow */
						{
							err = ParseFromEIT(secBufer, &mTSParser->pFollowEvent, &mTSParser->pf_eit_version_number,
								&mTSParser->pf_sec_mask, mTSParser->prog_num, &event_cnt);
							if(err)
							{
								DVBCORE_LOGD("parse EIT failed!\n");
							}
							mTSParser->pf_count = 0;
                            mTSParser->event_total_count += event_cnt;
							DVBCORE_LOGD("parse service_id %d EIT follow info finish!", service_id);
						}
                        if(sec_num == last_section_number)
                        {
							DVBCORE_LOGD("parse service_id %d EIT pf info finish!", service_id);
                            mTSParser->pf_parse_end = 1;
                        }
					}
				}
#endif
			}
			else
			{
				//DVBCORE_LOGW("other TS event.\n");
			}

			PSISectionClear(section);
			break;
			
		case SDT_BAT_PID:
			if(table_id == 0x42)
			{
				err = ParseFromSDT(secBufer, &mTSParser->pProgram, &mTSParser->sdt_version_number);
				if(!err)
				{
					//DVBCORE_LOGD("parse SDT successfully!\n");
					pthread_mutex_lock(&mTSParser->mutex); /*lock*/
					pthread_cond_signal(&mTSParser->cond);
					mTSParser->bIsEndSdtSection = 1;
					pthread_mutex_unlock(&mTSParser->mutex); /*unlock*/
				}
			}
			PSISectionClear(section);			
			break;
		case TDT_TOT_PID:
			err = ParseFromTDT(secBufer, mTSParser->CurTime);
			if(!err)
			{
				DVBCORE_LOGV("parse TDT successfully!\n");
			}
			PSISectionClear(section);
			
			break;
		case CAT_PID:
			err = ParseFromCAT(secBufer,&mTSParser->pProgram, &mTSParser->cat_version_number);
			if(!err)
			{				
				//DVBCORE_LOGD("parse CAT successfully!\n");
				//pthread_mutex_lock(&mTSParser->mutex); /*lock*/
				//pthread_cond_signal(&mTSParser->cond);
				mTSParser->bIsEndCatSection = 1;				
				//pthread_mutex_unlock(&mTSParser->mutex); /*unlock*/
			}
			PSISectionClear(section);
			break;
		default:
			//DVBCORE_LOGD("begin parse PMT section, pid: %d!\n", mTSParser->pProgram->uPMT_PID);
			err = ParseFromPMT(secBufer,&mTSParser->pProgram, &mTSParser->pmt_version_number);
			if(!err)
			{
				pthread_mutex_lock(&mTSParser->mutex); /*lock*/
				pthread_cond_signal(&mTSParser->cond);
				mTSParser->bIsEndPmtSection = 1;
				pthread_mutex_unlock(&mTSParser->mutex); /*unlock*/
			}
			PSISectionClear(section);
			break;
	}
	
_exit:
	
	DVBCoreBitReaderDestroy(sectionBits);
	return err;
}


status_t parsePID(TSParser *mTSParser, DVBCoreBitReaderT *br, unsigned PID,
	unsigned continuity_counter, unsigned payload_unit_start_indicator)
{	
	DVBCORE_UNUSE(continuity_counter);
	status_t err = SUCCESS;
    PSISection *section = findPSISectionByPID(mTSParser, PID);
	
    if (section) 
	{
        if (payload_unit_start_indicator)
        {
			//unsigned skip = DVBCoreBitReaderGetBits(br, 8);/*pointer_field*/
			uint32 skip = DVBCoreBitReaderGetBits(br, 8);
			uint32 end  = DVBCoreBitReaderNumBitsLeft(br) / 8;
			if(skip > end)
			{
				DVBCORE_LOGW("DVBCoreBitReaderNumLeft: %d", end);				
				return SUCCESS;
			}
			
            if(skip > 0)
            {
                if(PSISectionIsEmpty(section))
                {
					if(DvbCoreBitReaderSkipBitsSafe(br, skip * 8) < 0)
					{
						return SUCCESS;
					}
                }
                else
                {
                    PSISectionAppend(section, DVBCoreBitReaderData(br), skip);
                    err = parsePSISection(mTSParser, section);
                    DVBCORE_LOGV("err = %d", err);
					if(err == SKIP)
						return SUCCESS;
                    DVBCoreBitReaderSkipBits(br, skip * 8);
                }
            }
            section->mPayloadStarted = DVBCORE_TRUE;
        }

        if (!section->mPayloadStarted)
        {
            return SUCCESS;
        }

        DVBCORE_CHECK((DVBCoreBitReaderNumBitsLeft(br) % 8) == 0);
		PSISectionAppend(section, DVBCoreBitReaderData(br), 
			DVBCoreBitReaderNumBitsLeft(br) / 8);

        if (!PSISectionIsComplete(section)) 
		{
            return SUCCESS;
        }
		err = parsePSISection(mTSParser, section);
        return err;
    }
    return SUCCESS;
}

status_t parseSectionPacket(TSParser *mTSParser, int mSectionPid, const uint8 *data, int size)
{
    if(mTSParser->bIsEndEitSection == 1)
    {
        return SUCCESS;
    }

    PSISection * section = findPSISectionByPID(mTSParser, mSectionPid);
    if(section == NULL)
        return FAIL;

	DVBCoreBitReaderT *br = DVBCoreBitReaderCreate(data, size);
    PSISectionAppend(section, DVBCoreBitReaderData(br), size);

    status_t err = parsePSISection(mTSParser, section);
	DVBCoreBitReaderDestroy(br);
    return err;
}

static dvbcore_err SetPSISection(TSParser *mTSParser, unsigned PID)
{
    DVBCORE_CHECK(mTSParser);
    PSISection *section = DVBCoreMalloc(sizeof(PSISection));
    DVBCORE_FORCE_CHECK(section);
    section->PID = PID;
    section->mBuffer = DVBCoreBufferCreate(NULL, 1024, NULL, 0);
    section->mPayloadStarted = DVBCORE_FALSE;
    DVBCoreListAddTail(&section->node, &mTSParser->mPSISections);    
    return DVBCORE_SUCCESS;
}

TSParser * TSParserOpen(void)
{
    TSParser *mTSParser = DVBCoreMalloc(sizeof(TSParser));
    memset(mTSParser, 0x00, sizeof(TSParser));	
	return mTSParser;
}

void TSParserInit(TSParser *mTSParser)
{
	int i = 0;
    DVBCoreListInit(&mTSParser->mPSISections);

	SetPSISection(mTSParser, PAT_PID);
	SetPSISection(mTSParser, SDT_BAT_PID);
	SetPSISection(mTSParser, TDT_TOT_PID);
	SetPSISection(mTSParser, NIT_PID);
	SetPSISection(mTSParser, CAT_PID);
	SetPSISection(mTSParser, EIT_PID);

    mTSParser->enablePid[PAT_PID] = 1;
    mTSParser->enablePid[SDT_BAT_PID] = 1;
	mTSParser->enablePid[TDT_TOT_PID] = 1;
	mTSParser->enablePid[NIT_PID] = 1;
	mTSParser->enablePid[CAT_PID] = 1;
	mTSParser->enablePid[EIT_PID] = 1;


    mTSParser->pat_version_number = (unsigned char)-1;	
    mTSParser->pmt_version_number = (unsigned char)-1;
	mTSParser->sdt_version_number = (unsigned char)-1;
	mTSParser->nit_version_number = (unsigned char)-1;
	mTSParser->cat_version_number = (unsigned char)-1;
	mTSParser->pf_eit_version_number = (unsigned char)-1;

	mTSParser->pProgram = NULL;
	for(i = 0;i < EIT_SCHEDULE_TABLE_ID_NUM;i++)
	{
        mTSParser->schdl_count[i] = 0;
        mTSParser->schdl_parse_end[i] = 0;
		mTSParser->pProgramEvent[i] = NULL;
		mTSParser->schdl_eit_version_number[i] = (unsigned char)-1;
	}
	memset(mTSParser->schdl_sec_mask, 0, EIT_SCHEDULE_TABLE_ID_NUM*32*sizeof(SCH_SEC_MASK));
	memset(&mTSParser->pf_sec_mask, 0, sizeof(SCH_SEC_MASK));
	mTSParser->pPresentEvent = NULL;
	mTSParser->pFollowEvent = NULL;
    pthread_mutex_init(&mTSParser->mutex, NULL);
    pthread_cond_init(&mTSParser->cond, NULL);
	
	return;
}

int TSParserClose(TSParser *mTSParser)
{
	if(mTSParser)
	{
		DestroyPSISections(mTSParser);
		pthread_mutex_destroy(&mTSParser->mutex);
		pthread_cond_destroy(&mTSParser->cond);
		DVBCoreFree(mTSParser);	
	}
	return SUCCESS;
}
