/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : GetSection.c
* Description : using the file for get the section of the PID package which we want
* Version : 1.0
* History :
* xhw : 2015-9-11 Create
*********************************************************************************************************************************/

#ifndef PARSENIT_H_
#define PARSENIT_H_
#define DESCRIPTOR_MAX_LENGTH_4096 4096

typedef struct TS_NIT_TRANSPORT
{

	unsigned int transport_stream_id :16;
	unsigned int original_network_id :16;
	unsigned int reserved_future_use :4;
	unsigned int transport_descriptors_length :12;
	unsigned char descriptor[DESCRIPTOR_MAX_LENGTH_4096];
	struct TS_NIT_TRANSPORT *next;


} TS_NIT_TRANSPORT;

typedef struct TS_NIT
{

	unsigned int table_id :8;
	unsigned int section_syntax_indicator :1;
	unsigned int reserved_future_use_1 :1;
	unsigned int reserved_1 :2;
	unsigned int section_length :12;
	unsigned int network_id :16;
	unsigned int reserved_2 :2;
	unsigned int version_number :5;
	unsigned int current_next_indicator :1;
	unsigned int section_number :8;
	unsigned int last_section_number :8;
	unsigned int reserved_future_use_2 :4;
	unsigned int network_descriptors_length :12;
	unsigned char network_descriptor[DESCRIPTOR_MAX_LENGTH_4096];
	unsigned int reserved_future_use_3 :4;
	unsigned int transport_stream_loop_length :12;

	TS_NIT_TRANSPORT *pNitTranHead;

	unsigned CRC_32 :32;

} TS_NIT;

/*********************************************************************************************************************************
* Function Name : ParseFromNIT
* Description : parse the NIT table
* Parameters :
*p -- the buffer of section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromNIT(unsigned char *p, unsigned char* pVersionNumber);

#endif /* PARSENIT_H_ */