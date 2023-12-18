/********************************************************************************************************************************
 * All Rights GLOBALSAT Reserved            *
 ********************************************************************************************************************************
 ********************************************************************************************************************************
 * Filename   : Descriptor.c
 * Description  : the function about the descriptor
 * Version   : 1.0
 * History      :
 * xhw : 2015-9-15  Create
 *********************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "DVBCoreDescriptor.h"
#include "DVBCoreMemory.h"
#include "CharSetConvert.h"

#define PMT_CA_IDENTIFIER_DESCRIPTOR 0x09
#define SDT_SERVICE_DESCRIPTOR 0x48
#define EIT_SHORT_EVENT_DESCRIPTOR 0x4d
#define	EIT_EXTENTED_EVENT_DESCRIPTOR 	0x4E
#define COMPONENT_DESCRIPTOR			0x50
#define CONTENT_DESCRIPTOR			0x54
#define PARENTAL_RATING_DESCRIPTOR	0x55
#define TELE_TEXT_DESCRIPTOR			0x57
#define NIT_STATELLITE_DELIVERY_SYSTEM_DESCRIPTOR 0x43
#define NIT_TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR 0x5A
#define NIT_NETWORK_NAME_DESCRIPTOR 0x40
#define NIT_SERVICE_LIST_DESCRIPTOR 0x41
#define TOT_LOCAL_TIME_OFFSET_DESCRIPTOR 0x58


/*********************************************************************************************************************************

*********************************************************************************************************************************/

#define CAHR_CONVERT_ENABLE (1)

static int isgbk(char *s, size_t ns)
{
	if(ns > 2 && (uint8_t)*s >= 0x81 && (uint8_t)*s <= 0xfe
	&& (((uint8_t)*(s+1) >= 0x80 && (uint8_t)*(s+1) <= 0x7e)
		|| ((uint8_t)*(s+1) >= 0xa1 && (uint8_t)*(s+1) <= 0xfe)))
    {
		return 1;
    }
    return 0;
}

/*********************************************************************************************************************************

*********************************************************************************************************************************/

static int isutf8(char *s, size_t ns)
{
	uint8_t x = 0, i = 0, j = 0, nbytes = 0, n = 0;
	for(i = 1; i < 7; i++)
	{
		x = (uint8_t)(255 << i);
		if(((uint8_t)*s & x) == x)
		{
	        n = nbytes = (8 - i);
	        for(j = 0; (j < nbytes && j < ns); j++)
	        {
                if((uint8_t)s[j] <= 0x80 && (uint8_t)s[j] >= 0xc0)break;
                else n--;
	        }
	        if(n == 0) return nbytes;
		}
	}
	return 0;
}

static const char *type2str(TextFormat eTextFormat)
{
	const char*    str;

    switch(eTextFormat)
    {
        case TEXT_FORMAT_UTF8:
             str = CHARSET_UTF_8;
             break;

        case TEXT_FORMAT_GB2312:
             str = CHARSET_HZ_GB_2312;
             break;

        case TEXT_FORMAT_UTF16LE:
             str = CHARSET_UTF_16LE;
             break;

        case TEXT_FORMAT_UTF16BE:
             str = CHARSET_UTF_16BE;
             break;

        case TEXT_FORMAT_UTF32LE:
             str = CHARSET_UTF_32LE;
             break;

        case TEXT_FORMAT_UTF32BE:
             str = CHARSET_UTF_32BE;
             break;

        case TEXT_FORMAT_BIG5:
             str = CHARSET_BIG5;
             break;

        case TEXT_FORMAT_GBK:
             str = CHARSET_GBK;
             break;

        case TEXT_FORMAT_ANSI:  //can not match,set to "UTF-8"
             str = CHARSET_UTF_8;
             break;

        default:
             str = CHARSET_UNKNOWN;
             break;
    }

    return str;
}

/*********************************************************************************************************************************
 * Function Name : verifyTextType
 * Description   : verify text codec type
 * Parameters    :
 * src -- output buffer
 * pCode_type -- output buffer size
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
static int verifyTextType(unsigned char* src, unsigned int length, unsigned char* pCode_type)
{
	TextFormat		code_type;
	unsigned char	first_byte;
	unsigned char*	ptr;

	ptr 		= src;
	first_byte	= ptr[0];
	code_type	= TEXT_FORMAT_UNKNOWN;
	if(first_byte < 0x20)
	{
		switch(first_byte)
		{
			case 0x11:
			{
				code_type = TEXT_FORMAT_UTF16BE;
				break;
			}

			case 0x12:
			{
				code_type = TEXT_FORMAT_GBK;
				break;
			}

			case 0x13:
			{
				code_type = TEXT_FORMAT_GB2312;
				break;
			}

			case 0x14:
			{
				code_type = TEXT_FORMAT_BIG5;
				break;
			}

			case 0x15:
			{
				code_type = TEXT_FORMAT_UTF8;
				break;
			}

			default:
			{
				printf("unknown text code type!\n");
				break;
			}
		}
	}

    //logd("first detect, text code type [%d %s] !", code_type, type2str(code_type));

    if(code_type == TEXT_FORMAT_UNKNOWN)
    {
        if(isgbk((char*)ptr,(size_t)length))
            code_type = TEXT_FORMAT_GBK;
        else if(isutf8((char*)ptr,(size_t)length))
            code_type = TEXT_FORMAT_UTF8;
        else
        {
            printf("set default type GBK !\n");
            code_type = TEXT_FORMAT_GBK;//default set GBK
        }
    }

	*pCode_type = code_type;
	//logd("final detect, text code type [%d %s]", *pCode_type, type2str(*pCode_type));

	return 0;
}

/*********************************************************************************************************************************
 * Function Name : UtilsConvertUnicode
 * Description   : convert text code type to UTF-8
 * Parameters    :
 * pOutBuf -- output buffer
 * nOutBufSize -- output buffer size
 * pInBuf -- input buffer
 * nInBufSize -- input buffer size
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
static int UtilsConvertUnicode(char*	pOutBuf,
                               unsigned int nOutBufSize,
                               const char*	pInBuf,
                               unsigned int nInBufSize)
{
    int  ret    = 0;
    int  nOutSize = nOutBufSize;
    char* pInBufActual = NULL;
    TextFormat eInputTextFormat = TEXT_FORMAT_UNKNOWN;

	//logd("event_name_char: %02x %02x %02x %02x %02x %02x %02x %02x !", pInBuf[0], pInBuf[1],
         //pInBuf[2], pInBuf[3],pInBuf[4], pInBuf[5],pInBuf[6], pInBuf[7]);

    verifyTextType((unsigned char*)pInBuf, nInBufSize, (unsigned char*)&eInputTextFormat);
    //logd("input text format %d , %s !", eInputTextFormat, type2str(eInputTextFormat));

    if(eInputTextFormat == TEXT_FORMAT_UTF16BE)
    {
        nInBufSize--;
        pInBufActual = (char*)pInBuf + 1;//skip 0x11
        ret = utf16utf8(pInBufActual, &nInBufSize, pOutBuf, &nOutBufSize);
    }
    else if(eInputTextFormat == TEXT_FORMAT_GBK)
    {
        ret = gbk2utf8((char*)pInBuf, &nInBufSize, pOutBuf, &nOutBufSize);
    }

    if(ret < 0)
    {
        printf("convert eInputTextFormat %s failed, use input value !", type2str(eInputTextFormat));
        memset(pOutBuf, 0, nOutSize);
        memcpy(pOutBuf, pInBuf, nOutSize);
    }

    //logd("TimedText::convertUnicode in = %s, out = %s, nOutBufSize %d, actual_out_len = %d",
        //pInBuf, pOutBuf, nOutBufSize, strlen(pOutBuf));

    return ret;
}

/*********************************************************************************************************************************
 * Function Name : GetDescriptorTagPosition
 * Description   : get the descriptor tag position which we want
 * Parameters    :
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * uiDescriptorTag -- the descriptor tag type (0x4d)
 * Returns       : -1 -- failure >0 -- success
 *********************************************************************************************************************************/
static int GetDescriptorTagPosition(unsigned char *pucDescriptor, unsigned int uiDescriprotLength, unsigned int uiDescriptorTag)
{
 
	int iPosition = 0;
	unsigned int iDescriptorTagLength = 0;
	unsigned int uiDescriptorTagTemp = 0;
	 
	while (iPosition < (int)uiDescriprotLength)
	{
	 
		uiDescriptorTagTemp = pucDescriptor[iPosition];
		if (uiDescriptorTag == uiDescriptorTagTemp)
		{
		 
			return iPosition;
		 
		}
		iDescriptorTagLength = pucDescriptor[iPosition + 1];
		iPosition = iPosition + 2 + iDescriptorTagLength;
	 
	}
	return -1;
 
}
 
/*********************************************************************************************************************************
 * Function Name : GetServiceDescriptor
 * Description   : get the service descriptor information
 * Parameters    :
 * pServiceDescriptor -- the service descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * uiDescriptorTag -- the descriptor tag type (0x48)
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetServiceDescriptor(SERVICE_DESCRIPTOR *pServiceDescriptor, unsigned char *pucDescriptor, unsigned int uiDescriprotLength)
{
 
	int iServiceDespritorPosition = -1;
	 
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n\n");
		return -1;
	 
	}
	iServiceDespritorPosition = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, SDT_SERVICE_DESCRIPTOR);
	if (-1 == iServiceDespritorPosition)
	{
	 
		printf("no service_descriptor information\n\n");
		return -1;
	 
	}

	pServiceDescriptor->descriptor_tag = pucDescriptor[iServiceDespritorPosition];
	pServiceDescriptor->descriptor_length = pucDescriptor[iServiceDespritorPosition + 1];
	//logd("descriptor_tag: %x\n", pServiceDescriptor->descriptor_tag);
	pServiceDescriptor->service_type = pucDescriptor[iServiceDespritorPosition + 2];
	pServiceDescriptor->service_provider_name_length = pucDescriptor[iServiceDespritorPosition + 3];

	memcpy(pServiceDescriptor->service_provider_name, pucDescriptor + iServiceDespritorPosition + 4,
	        pServiceDescriptor->service_provider_name_length);
	
	pServiceDescriptor->service_provider_name[pServiceDescriptor->service_provider_name_length] = '\0';
	pServiceDescriptor->service_name_length = pucDescriptor[4 + pServiceDescriptor->service_provider_name_length];

	memcpy(pServiceDescriptor->service_name,
	        pucDescriptor + iServiceDespritorPosition + 5 + pServiceDescriptor->service_provider_name_length,
	        pServiceDescriptor->service_name_length);

	pServiceDescriptor->service_name[pServiceDescriptor->service_name_length] = '\0';
	return 0;
 
}
 
/*********************************************************************************************************************************
 * Function Name : GetCaIdentifierDescriptor
 * Description   : get the CA identifier descriptor information
 * Parameters    :
 * pCaIdentifierDescriptor -- the service descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetCaIdentifierDescriptor(CA_IDENTIFIER_DESCRIPTOR *pCaIdentifierDescriptor, unsigned char *pucDescriptor,
        unsigned int uiDescriprotLength)
{
 
	int iCaIdentifierPosition = -1;
	 
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n\n");
		return -1;
	 
	}
	iCaIdentifierPosition = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, PMT_CA_IDENTIFIER_DESCRIPTOR);
	if (-1 == iCaIdentifierPosition)
	{
	 
		//printf("no ca_identifier_descriptor information \n\n");
		return -1;
	 
	}
	pCaIdentifierDescriptor->descriptor_tag = pucDescriptor[iCaIdentifierPosition];
	pCaIdentifierDescriptor->descriptor_length = pucDescriptor[iCaIdentifierPosition + 1];
	pCaIdentifierDescriptor->ca_system_id = (pucDescriptor[iCaIdentifierPosition + 2] << 8) | pucDescriptor[iCaIdentifierPosition + 3];
	pCaIdentifierDescriptor->reserved = pucDescriptor[iCaIdentifierPosition + 4] >> 5;
	pCaIdentifierDescriptor->ca_pid = ((pucDescriptor[iCaIdentifierPosition + 4] & 0x1F) << 8) | pucDescriptor[iCaIdentifierPosition + 5];
	return 0;
 
}
 
/*********************************************************************************************************************************
 * Function Name : GetShortEventDescriptor
 * Description   : get the service descriptor information
 * Parameters    :
 * pShortEventDescriptor -- the service descriptor structure
 * pucDescriptors -- the descriptor buffer
 * iDescriprotLength -- the descriptor length
 * uiDescriptorTag -- the descriptor tag type (0x48)
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetShortEventDescriptor(SHORT_EVENT_DESCRIPTOR *pShortEventDescriptor, unsigned char *pucDescriptor, unsigned int uiDescriprotLength)
{
	 
	int iShotEventDespritorPosition = -1;
	char event_name_char[DC_NAME_LENGTH];
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n\n");
		return -1;
	 
	}
	iShotEventDespritorPosition = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, EIT_SHORT_EVENT_DESCRIPTOR);
	if (-1 == iShotEventDespritorPosition)
	{
	 
		printf("no short_event_descriptor information\n\n");
		return -1;
	 
	}
	pShortEventDescriptor->descriptor_tag = pucDescriptor[iShotEventDespritorPosition];
	pShortEventDescriptor->descriptor_length = pucDescriptor[iShotEventDespritorPosition + 1];
	pShortEventDescriptor->ISO_639_language_code = (pucDescriptor[iShotEventDespritorPosition + 2] << 16)
	        | (pucDescriptor[iShotEventDespritorPosition + 3] << 8) | (pucDescriptor[iShotEventDespritorPosition + 4]);

	pShortEventDescriptor->event_name_length = pucDescriptor[iShotEventDespritorPosition + 5];
	memcpy(pShortEventDescriptor->event_name_char, pucDescriptor + iShotEventDespritorPosition + 6,
	        pShortEventDescriptor->event_name_length);
	pShortEventDescriptor->event_name_char[pShortEventDescriptor->event_name_length] = '\0';

	pShortEventDescriptor->text_length = pucDescriptor[6 + pShortEventDescriptor->event_name_length];
	memcpy(pShortEventDescriptor->text_char, pucDescriptor + iShotEventDespritorPosition + 7 + pShortEventDescriptor->event_name_length,
	        pShortEventDescriptor->text_length);
	pShortEventDescriptor->text_char[pShortEventDescriptor->text_length] = '\0';

	#if CAHR_CONVERT_ENABLE
	memcpy(event_name_char, pShortEventDescriptor->event_name_char,
	        pShortEventDescriptor->event_name_length);
	event_name_char[pShortEventDescriptor->event_name_length] = '\0';

	UtilsConvertUnicode(pShortEventDescriptor->event_name_char, DC_NAME_LENGTH, event_name_char, pShortEventDescriptor->event_name_length);
	#endif

	return 0;
}

/*********************************************************************************************************************************
 * Function Name : GetNetworkNameDescriptor
 * Description   : get the service descriptor information
 * Parameters    :
 * pNetworkNameDescriptor -- the service descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetNetworkNameDescriptor(NETWORK_NAME_DESCRIPTOR *pNetworkNameDescriptor, unsigned char *pucDescriptor,
        unsigned int uiDescriprotLength)
{
 
	int iNetworkNamePosition = -1;
	 
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n");
		return -1;
	 
	}
	iNetworkNamePosition = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, NIT_NETWORK_NAME_DESCRIPTOR);
	if (-1 == iNetworkNamePosition)
	{
	 
		printf("no network_name_descriptor information\n");
		return -1;
	 
	}
	pNetworkNameDescriptor->descriptor_tag = pucDescriptor[iNetworkNamePosition];
	pNetworkNameDescriptor->descriptor_length = pucDescriptor[iNetworkNamePosition + 1];
	memcpy(pNetworkNameDescriptor->network_name, pucDescriptor + iNetworkNamePosition + 2, pNetworkNameDescriptor->descriptor_length);
	pNetworkNameDescriptor->network_name[pNetworkNameDescriptor->descriptor_length] = '\0';
	return 0;
 
}
 
/*********************************************************************************************************************************
 * Function Name : GetSatelliteDeliverySystemDescriptor
 * Description   : get the service descriptor information
 * Parameters    :
 * pSatellitDeliveryDescriptor -- the service descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetSatelliteDeliverySystemDescriptor(SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR *pSatellitDeliveryDescriptor, unsigned char *pucDescriptor,
        unsigned int uiDescriprotLength)
{
 
	int iSatelliteDeliveryPosition = -1;
	 
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n");
		return -1;
	 
	}
	iSatelliteDeliveryPosition = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, NIT_STATELLITE_DELIVERY_SYSTEM_DESCRIPTOR);
	if (-1 == iSatelliteDeliveryPosition)
	{
	 
		//printf("no service_satellite_delivery_system_descriptor information\n");
		return -1;
	 
	}
	pSatellitDeliveryDescriptor->descriptor_tag = pucDescriptor[iSatelliteDeliveryPosition];
	pSatellitDeliveryDescriptor->descriptor_length = pucDescriptor[iSatelliteDeliveryPosition + 1];
	pSatellitDeliveryDescriptor->frequency = (pucDescriptor[iSatelliteDeliveryPosition + 2] << 24)
	        									| (pucDescriptor[iSatelliteDeliveryPosition + 3] << 16) 
	        									| (pucDescriptor[iSatelliteDeliveryPosition + 4] << 8)
	        									| (pucDescriptor[iSatelliteDeliveryPosition + 5]);
	pSatellitDeliveryDescriptor->orbital_position = (pucDescriptor[iSatelliteDeliveryPosition + 6] << 8)
	        										  | pucDescriptor[iSatelliteDeliveryPosition + 7];
	pSatellitDeliveryDescriptor->west_east_flag = pucDescriptor[iSatelliteDeliveryPosition + 8] >> 7;
	pSatellitDeliveryDescriptor->polarization = (pucDescriptor[iSatelliteDeliveryPosition + 8] & 0x7F) >> 5;
	pSatellitDeliveryDescriptor->roll_off = (pucDescriptor[iSatelliteDeliveryPosition + 8] & 0x1F) >> 3;
	pSatellitDeliveryDescriptor->modulation_system = (pucDescriptor[iSatelliteDeliveryPosition + 8] & 0x07) >> 2;
	pSatellitDeliveryDescriptor->modulation_type = pucDescriptor[iSatelliteDeliveryPosition + 8] & 0x03;
	pSatellitDeliveryDescriptor->symbol_rate = (pucDescriptor[iSatelliteDeliveryPosition + 9] << 20)
	        									| (pucDescriptor[iSatelliteDeliveryPosition + 10] << 12) 
	        									| (pucDescriptor[iSatelliteDeliveryPosition + 11] << 4)
	        									| (pucDescriptor[iSatelliteDeliveryPosition + 12] >> 4);
	pSatellitDeliveryDescriptor->FEC_inner = pucDescriptor[iSatelliteDeliveryPosition + 12] & 0x0F;
	return 0;
 
}

/*********************************************************************************************************************************
 * Function Name : GetTerrestrialDeliverySystemDescriptor
 * Description   : get the service descriptor information
 * Parameters    :
 * pTerDeliveryDescriptor -- the service descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetTerrestrialDeliverySystemDescriptor(TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR *pTerDeliveryDescriptor, unsigned char *pucDescriptor,
        unsigned int uiDescriprotLength)
{
 
	int Position = -1;
	 
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n\n");
		return -1;
	 
	}
	Position = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, NIT_TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR);
	if (-1 == Position)
	{
	 
		//printf("no service_satellite_delivery_system_descriptor information\n\n");
		return -1;
	 
	}
	pTerDeliveryDescriptor->descriptor_tag = pucDescriptor[Position];
	pTerDeliveryDescriptor->descriptor_length = pucDescriptor[Position + 1];
	pTerDeliveryDescriptor->centre_frequency  = (pucDescriptor[Position + 2] << 24)
	        									| (pucDescriptor[Position + 3] << 16) 
	        									| (pucDescriptor[Position + 4] << 8)
	        									| (pucDescriptor[Position + 5]);
	
	pTerDeliveryDescriptor->bandwidth = (pucDescriptor[Position + 6] >> 5) & 0x07;
	pTerDeliveryDescriptor->reserved_future_use = pucDescriptor[Position + 6] & 0x1F;
	
	pTerDeliveryDescriptor->constellation= (pucDescriptor[Position + 7] >> 6) & 0x03;
	pTerDeliveryDescriptor->hierarchy_information = (pucDescriptor[Position + 7] >> 3) & 0x07;
	pTerDeliveryDescriptor->code_rate_HP_stream = pucDescriptor[Position + 7] & 0x07;
	
	pTerDeliveryDescriptor->code_rate_LP_stream = (pucDescriptor[Position + 8] >> 5) & 0x07;
	pTerDeliveryDescriptor->guard_interval = (pucDescriptor[Position + 8] >> 3) & 0x03;
	pTerDeliveryDescriptor->transmission_mode = (pucDescriptor[Position + 8] >> 1) & 0x03;	
	pTerDeliveryDescriptor->other_frequency_flag = pucDescriptor[Position + 8] & 0x01;

	pTerDeliveryDescriptor->reserved_future_use1 = (pucDescriptor[Position + 9] << 24)
	        									| (pucDescriptor[Position + 10] << 16) 
	        									| (pucDescriptor[Position + 11] << 8)
	        									| (pucDescriptor[Position + 12]);
	return 0; 
 
} 
/*********************************************************************************************************************************
 * Function Name : GetServiceListDescriptor
 * Description   : get the service list descriptor information
 * Parameters    :
 * pServiceListDescriptor -- the service descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetServiceListDescriptor(SERVICE_LIST_DESCRIPTOR *pServiceListDescriptor, unsigned char *pucDescriptor,
        unsigned int uiDescriprotLength)
{
 
	int iServiceListPosition = -1;
	int iTemp = 0;
	SERVICE_LIST *pServiceListTemp = NULL;
	SERVICE_LIST *pServiceListForMalloc = NULL;
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n\n");
		return -1;
	 
	}
	iServiceListPosition = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, NIT_SERVICE_LIST_DESCRIPTOR);
	if (-1 == iServiceListPosition)
	{
	 
		printf("no service_list_descriptor\n\n");
		return -1;
	 
	}
	pServiceListDescriptor->descriptor_tag = pucDescriptor[iServiceListPosition];
	pServiceListDescriptor->descriptor_length = pucDescriptor[iServiceListPosition + 1];
	for (iTemp = 2 + iServiceListPosition; iTemp < (int)(pServiceListDescriptor->descriptor_length + iServiceListPosition + 1);
	     iTemp += 3)
	{
	 
		pServiceListForMalloc = (SERVICE_LIST *)DVBCoreMalloc(sizeof(SERVICE_LIST));
		pServiceListForMalloc->service_id = (pucDescriptor[iTemp] << 8) | pucDescriptor[iTemp + 1];
		pServiceListForMalloc->service_type = pucDescriptor[iTemp + 2];
		pServiceListForMalloc->next = NULL;
		if (NULL == pServiceListDescriptor->pServieListHead)
		{
		 
			pServiceListDescriptor->pServieListHead = pServiceListForMalloc;
		 
		}
		else
		{
		 
			pServiceListTemp->next = pServiceListForMalloc;
		 
		}
		pServiceListTemp = pServiceListForMalloc;
	 
	}
	return 0;
	 
}

/*********************************************************************************************************************************
 * Function Name : GetLocalTimeOffsetDescriptor
 * Description   : get the local time offset descriptor information
 * Parameters    :
 * pLocalTimeOffsetDescriptor -- the local time offset descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetLocalTimeOffsetDescriptor(LOCAL_TIME_OFFSET_DESCRIPTOR *pLocalTimeOffsetDescriptor, unsigned char *pucDescriptor, unsigned int uiDescriprotLength)
{

	int position = -1;
	 
	if (NULL == pucDescriptor)
	{
	 
		printf("no descriptor information\n\n");
		return -1;
	 
	}
	position = GetDescriptorTagPosition(pucDescriptor, uiDescriprotLength, TOT_LOCAL_TIME_OFFSET_DESCRIPTOR);
	if (-1 == position)
	{
	 
		printf("no local_time_offset_descriptor information \n\n");
		return -1;
	 
	}	
	pLocalTimeOffsetDescriptor->descriptor_tag = pucDescriptor[position];
	pLocalTimeOffsetDescriptor->descriptor_length = pucDescriptor[position + 1];
	pLocalTimeOffsetDescriptor->country_code = (pucDescriptor[position + 2] << 8) | pucDescriptor[position + 3];
	return 0;
}

 
/*********************************************************************************************************************************
 * Function Name : FreeServerListLink
 * Description   : free the memory of the link
 * Parameters    :
 * pServerListHead -- the link head
 * Returns       : void
 *********************************************************************************************************************************/
void FreeServerListLink(SERVICE_LIST **pServerListHead)
{
 
	SERVICE_LIST *pSeverListTemp = *pServerListHead;
	while (NULL != pSeverListTemp)
	{
	 
		pSeverListTemp = pSeverListTemp->next;
		DVBCoreFree(*pServerListHead);
		(*pServerListHead) = NULL;
	 
	}
 
}
