/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : Descriptor.c
* Description : the function about the descriptor
* Version : 1.0
* History :
* xhw : 2015-9-15 Create
*********************************************************************************************************************************/

#ifndef DESCRIPTOR_H_
#define DESCRIPTOR_H_

#define DC_NAME_LENGTH 512
#ifndef true
#define true 1
#endif

#ifndef false
#define false 1
#endif

#define CHARSET_UNKNOWN                        "UNKNOWN"               
#define CHARSET_BIG5                           "Big5"                           
#define CHARSET_GBK                            "GBK"
#define CHARSET_HZ_GB_2312                     "HZ-GB-2312"
#define CHARSET_UTF_16BE                       "UTF-16BE"
#define CHARSET_UTF_16LE                       "UTF-16LE"
#define CHARSET_UTF_32BE                       "UTF-32BE"
#define CHARSET_UTF_32LE                       "UTF-32LE"
#define CHARSET_UTF_8                          "UTF-8"
#define CHARSET_US_ASCII                       "US-ASCII" 

/*----------------------------------------------------------*/
/* network_name_descriptor descriptor_tag 0x40 */
/*----------------------------------------------------------*/
typedef struct CA_IDENTIFIER_DESCRIPTOR
{

	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	unsigned int ca_system_id :16;
	unsigned int reserved :3;
	unsigned int ca_pid :13;

} CA_IDENTIFIER_DESCRIPTOR;

/*----------------------------------------------------------*/
/* service_descriptor descriptor_tag 0x48 */
/*----------------------------------------------------------*/
typedef struct SERVICE_DESCRIPTOR
{

	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	unsigned int service_type :8;
	unsigned int service_provider_name_length :8;
	char service_provider_name[DC_NAME_LENGTH];
	unsigned int service_name_length :8;
	char service_name[DC_NAME_LENGTH];

} SERVICE_DESCRIPTOR;

/*----------------------------------------------------------*/
/* service_descriptor descriptor_tag 0x4d */
/*----------------------------------------------------------*/
typedef struct SHORT_EVENT_DESCRIPTOR
{

	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	unsigned int ISO_639_language_code :24;
	unsigned int event_name_length :8;
	char event_name_char[DC_NAME_LENGTH];
	unsigned int text_length :8;
	char text_char[DC_NAME_LENGTH];

} SHORT_EVENT_DESCRIPTOR;

/*----------------------------------------------------------*/
/* network_name_descriptor descriptor_tag 0x40 */
/*----------------------------------------------------------*/
typedef struct NETWORK_NAME_DESCRIPTOR
{

	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	char network_name[DC_NAME_LENGTH];

} NETWORK_NAME_DESCRIPTOR;

/*----------------------------------------------------------*/
/* satellite_delivery_system_descriptor descriptor_tag 0x43 */
/*----------------------------------------------------------*/
typedef struct SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR
{

	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	unsigned int frequency :32;
	unsigned int orbital_position :16;
	unsigned int west_east_flag :1;
	unsigned int polarization :2;
	unsigned int roll_off :2;
	unsigned int modulation_system :1;
	unsigned int modulation_type :2;
	unsigned int symbol_rate :28;
	unsigned int FEC_inner :4;

} SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR;

/*----------------------------------------------------------*/
/* terrestrial_delivery_system_descriptor descriptor_tag 0x5A */
/*----------------------------------------------------------*/
typedef struct TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR
{

	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	unsigned int centre_frequency :32;
	unsigned int bandwidth :3;
	unsigned int reserved_future_use :5;
	unsigned int constellation :2;
	unsigned int hierarchy_information :3;
	unsigned int code_rate_HP_stream :3;
	unsigned int code_rate_LP_stream :3;
	unsigned int guard_interval :2;
	unsigned int transmission_mode :2;
	unsigned int other_frequency_flag :1;
	unsigned int reserved_future_use1 :32;

} TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR;


/*----------------------------------------------------------*/
/* service_list_descriptor descriptor_tag 0x41 */
/*----------------------------------------------------------*/
typedef struct SERVICE_LIST
{

	unsigned int service_id :16;
	unsigned int service_type :8;
	struct SERVICE_LIST *next;

} SERVICE_LIST;

typedef struct SERVICE_LIST_DESCRIPTOR
{

	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	SERVICE_LIST *pServieListHead;

} SERVICE_LIST_DESCRIPTOR;

typedef struct LOCAL_TIME_OFFSET_DESCRIPTOR
{
	unsigned int descriptor_tag :8;
	unsigned int descriptor_length :8;
	unsigned int country_code :24;
	unsigned int country_region_id :6;
	unsigned int reserved :1;
	unsigned int local_time_offset_polarity :1;
	unsigned int local_time_offset :16;
	unsigned int time_of_change_mjd :16;
	unsigned int time_of_change_utc :24;
	unsigned int next_time_offset :16;

}LOCAL_TIME_OFFSET_DESCRIPTOR;

typedef enum TEXTFORMAT
{
    TEXT_FORMAT_UNKNOWN = 0,
    TEXT_FORMAT_UTF8,
    TEXT_FORMAT_GB2312,
    TEXT_FORMAT_UTF16LE,
    TEXT_FORMAT_UTF16BE,
    TEXT_FORMAT_UTF32LE,
    TEXT_FORMAT_UTF32BE,
    TEXT_FORMAT_BIG5,
    TEXT_FORMAT_GBK,
    TEXT_FORMAT_ANSI,
    //* TO BE ADD.    
}TextFormat;

/*********************************************************************************************************************************
* Function Name : GetCaIdentifierDescriptor
* Description : get the CA identifier descriptor information
* Parameters :
* pCaIdentifierDescriptor -- the service descriptor structure
* pucDescriptors -- the descriptor buffer
* uiDescriprotLength -- the descriptor length
* Returns : -1 -- failure 0 -- success
*********************************************************************************************************************************/
int GetCaIdentifierDescriptor(CA_IDENTIFIER_DESCRIPTOR *pCaIdentifierDescriptor, unsigned char *pucDescriptor,
unsigned int uiDescriprotLength);

/*********************************************************************************************************************************
* Function Name : GetServiceDescriptor
* Description : get the service descriptor information
* Parameters :
* pServiceDescriptor -- the service descriptor structure
* pucDescriptors -- the descriptor buffer
* uiDescriprotLength -- the descriptor length
* uiDescriptorTag -- the descriptor tag type (0x48)
* Returns : -1 -- failure 0 -- success
*********************************************************************************************************************************/
int GetServiceDescriptor(SERVICE_DESCRIPTOR *pServiceDescriptor, unsigned char *pucDescriptor, unsigned int uiDescriprotLength);

/*********************************************************************************************************************************
* Function Name : GetShortEventDescriptor
* Description : get the service descriptor information
* Parameters :
* pShortEventDescriptor -- the service descriptor structure
* pucDescriptors -- the descriptor buffer
* iDescriprotLength -- the descriptor length
* uiDescriptorTag -- the descriptor tag type (0x48)
* Returns : -1 -- failure 0 -- success
*********************************************************************************************************************************/
int GetShortEventDescriptor(SHORT_EVENT_DESCRIPTOR *pShortEventDescriptor, unsigned char *pucDescriptor, unsigned int uiDescriprotLength);

/*********************************************************************************************************************************
* Function Name : GetNetworkNameDescriptor
* Description : get the service descriptor information
* Parameters :
* pNetworkNameDescriptor -- the service descriptor structure
* pucDescriptors -- the descriptor buffer
* uiDescriprotLength -- the descriptor length
* Returns : -1 -- failure 0 -- success
*********************************************************************************************************************************/
int GetNetworkNameDescriptor(NETWORK_NAME_DESCRIPTOR *pNetworkNameDescriptor, unsigned char *pucDescriptor,
unsigned int uiDescriprotLength);

/*********************************************************************************************************************************
* Function Name : GetSatelliteDeliverySystemDescriptor
* Description : get the service descriptor information
* Parameters :
* pSatellitDeliveryDescriptor -- the service descriptor structure
* pucDescriptors -- the descriptor buffer
* uiDescriprotLength -- the descriptor length
* Returns : -1 -- failure 0 -- success
*********************************************************************************************************************************/
int GetSatelliteDeliverySystemDescriptor(SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR *pSatellitDeliveryDescriptor, unsigned char *pucDescriptor,
unsigned int uiDescriprotLength);

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
unsigned int uiDescriprotLength);


/*********************************************************************************************************************************
* Function Name : GetServiceListDescriptor
* Description : get the service list descriptor information
* Parameters :
* pServiceListDescriptor -- the service descriptor structure
* pucDescriptors -- the descriptor buffer
* uiDescriprotLength -- the descriptor length
* Returns : -1 -- failure 0 -- success
*********************************************************************************************************************************/
int GetServiceListDescriptor(SERVICE_LIST_DESCRIPTOR *pServiceListDescriptor, unsigned char *pucDescriptor,
unsigned int uiDescriprotLength);

/*********************************************************************************************************************************
 * Function Name : GetLocalTimeOffsetDescriptor
 * Description   : get the local time offset descriptor information
 * Parameters    :
 * pLocalTimeOffsetDescriptor -- the local time offset descriptor structure
 * pucDescriptors -- the descriptor buffer
 * uiDescriprotLength -- the descriptor length
 * Returns       : -1 -- failure 0 -- success
 *********************************************************************************************************************************/
int GetLocalTimeOffsetDescriptor(LOCAL_TIME_OFFSET_DESCRIPTOR *pLocalTimeOffsetDescriptor, unsigned char *pucDescriptor, unsigned int uiDescriprotLength);

/*********************************************************************************************************************************
* Function Name : FreeServerListLink
* Description : free the memory of the link
* Parameters :
* pServerListHead -- the link head
* Returns : void
*********************************************************************************************************************************/
void FreeServerListLink(SERVICE_LIST **pServerListHead);

#endif /* DESCRIPTOR_H_ */
