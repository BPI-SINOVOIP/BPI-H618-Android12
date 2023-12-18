/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseTDT.h
* Description : parse the TDT
* Version : 1.0
* History :
* xhw : 2015-9-25 Create
*********************************************************************************************************************************/

#ifndef PARSETDT_H_
#define PARSETDT_H_
#define DESCRIPTOR_MAX_LENGTH_4096 4096

typedef struct TS_TDT
{

	unsigned int table_id :8;
	unsigned int section_syntax_indicator :1;
	unsigned int reserved_future_use :1;
	unsigned int reserved_1 :2;
	unsigned int section_length :12;
	unsigned char UTC_time[5];

	unsigned char descriptor[DESCRIPTOR_MAX_LENGTH_4096];

} TS_TDT;

/*********************************************************************************************************************************
* Function Name : ParseFromTDT
* Description : parse the TDT table and put the information in program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromTDT(unsigned char *p, char *pCurTime);

#endif /* PARSETDT_H_ */
