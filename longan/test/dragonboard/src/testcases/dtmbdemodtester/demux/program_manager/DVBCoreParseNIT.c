/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseNIT.c
* Description : using the file for get the section of the PID package which we want
* Version : 1.0
* History :
* xhw : 2015-9-11 Create
*********************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVBCoreProgram.h"
#include "DVBCoreDescriptor.h"
#include "DVBCoreParseNIT.h"
#include "DVBCoreMemory.h"
/*********************************************************************************************************************************
* Function Name : InitialTsNitTransport
* Description : initial the TS_NIT_TRANSPORT
* Parameters :
* pNitStansport -- the TS_NIT_TRANSPORT
* Returns : void
*********************************************************************************************************************************/
static void InitialTsNitTransport(TS_NIT_TRANSPORT *pNitStansport)
{

	pNitStansport->transport_stream_id = 0x0000;
	pNitStansport->original_network_id = 0x0000;
	pNitStansport->reserved_future_use = 0x00;
	pNitStansport->transport_descriptors_length = 0x0000;
	memset(pNitStansport->descriptor, '\0', DESCRIPTOR_MAX_LENGTH_4096);
	pNitStansport->next = NULL;

}

/*********************************************************************************************************************************
* Function Name : PutInTsNitTransportLinkList
* Description : put the transport to link list
* Parameters :
* pNitStansportHead -- the link list header
* pNitStansport -- the NIT transport we should put
* Returns : void
*********************************************************************************************************************************/
static void PutInTsNitTransportLinkList(TS_NIT_TRANSPORT **pNitStansportHead, TS_NIT_TRANSPORT *pNitStansport)
{

	TS_NIT_TRANSPORT *pNitStansportTemp = *pNitStansportHead;

	if (NULL == (*pNitStansportHead))
	{
		(*pNitStansportHead) = pNitStansport;
	}
	else
	{
		while (NULL != pNitStansportTemp->next)
		{
			pNitStansportTemp = pNitStansportTemp->next;
		}
		pNitStansportTemp->next = pNitStansport;
	}

}

/*********************************************************************************************************************************
* Function Name : ParseNIT
* Description : parse the NIT table
* Parameters :
* pNIT -- the NIT
* p -- the buffer of NIT section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
static int ParseNIT(TS_NIT *pNIT, unsigned char *p, unsigned char* pVersionNumber)
{

	int nLen = 0;
	int nPos = 0;
	int descLen = 0;
	TS_NIT_TRANSPORT * pNitMalloc = NULL;
	
	pNIT->pNitTranHead = NULL;
	pNIT->table_id = p[0];
	if(pNIT->table_id != 0x40)
	{
		printf("other network!");
		return 0;
	}
	pNIT->section_syntax_indicator = p[1] >> 7;
	pNIT->reserved_future_use_1 = (p[1] >> 6) & 0x01;
	pNIT->reserved_1 = (p[1] >> 4) & 0x03;
	pNIT->section_length = ((p[1] & 0x0F) << 8) | p[2];
	pNIT->network_id = (p[3] << 8) | p[4];
	pNIT->reserved_2 = p[5] >> 6;
	pNIT->version_number = (p[5] >> 1) & 0x1F;
	pNIT->current_next_indicator = (p[5] << 7) >> 7;
    if(pNIT->version_number == *pVersionNumber || !pNIT->current_next_indicator)
    {
		return 0;
    }	
	pNIT->section_number = p[6];
	pNIT->last_section_number = p[7];
	pNIT->reserved_future_use_2 = p[8] >> 4;
	pNIT->network_descriptors_length = ((p[8] & 0x0F) << 8) | p[9];

	descLen = pNIT->network_descriptors_length;
	memcpy(pNIT->network_descriptor, p + 10, descLen);
	pNIT->network_descriptor[descLen] = '\0';
	pNIT->reserved_future_use_2 = p[10 + descLen] >> 4;
	pNIT->transport_stream_loop_length = ((p[10 + descLen] & 0x0F) << 8) | p[10 + descLen + 1];

	nLen = pNIT->section_length + 3;
	pNIT->CRC_32 = (p[nLen - 4] << 24) | (p[nLen - 3] << 16) | (p[nLen - 2] << 8) | p[nLen - 1];

	for (nPos = 10 + descLen + 2; nPos < nLen - 4;nPos += 6)
	{
		pNitMalloc = (TS_NIT_TRANSPORT *)DVBCoreMalloc(sizeof(TS_NIT_TRANSPORT));
		if (NULL == pNitMalloc)
		{
			printf("NIT Malloc failed.");
			return -1;
		}
		InitialTsNitTransport(pNitMalloc);
		pNitMalloc->transport_stream_id = (p[nPos] << 8) | p[nPos + 1];
		pNitMalloc->original_network_id = (p[nPos + 2] << 8) | p[nPos + 3];
		pNitMalloc->reserved_future_use = p[nPos + 4] >> 4;
		pNitMalloc->transport_descriptors_length = ((p[nPos + 4] & 0x0F) << 8) | p[nPos + 5];
		if (pNitMalloc->transport_descriptors_length != 0)
		{
			memcpy(pNitMalloc->descriptor, p + 6 + nPos, pNitMalloc->transport_descriptors_length);
			pNitMalloc->descriptor[pNitMalloc->transport_descriptors_length] = '\0';
			nPos += pNitMalloc->transport_descriptors_length;
		}
		PutInTsNitTransportLinkList(&(pNIT->pNitTranHead), pNitMalloc);

	}
	return 0;

}

/*********************************************************************************************************************************
* Function Name : FreeTsNitTransport
* Description : free the memory of the transport
* Parameters :
* TS_NIT_TRANSPORT -- the transport first address
* Returns : void
*********************************************************************************************************************************/
static void FreeTsNitTransport(TS_NIT_TRANSPORT **pNitTran)
{

	TS_NIT_TRANSPORT *pNitTranTemp = NULL;

	while (NULL != (*pNitTran))
	{
		pNitTranTemp = (*pNitTran)->next;
		DVBCoreFree(*pNitTran);
		(*pNitTran) = pNitTranTemp;
	}
	(*pNitTran) = NULL;

}

/*********************************************************************************************************************************
* Function Name : DeleteHex0x
* Description : delete the hex 0x(0x47(hex) -> 47(decimal))
* Parameters :
*ucSource -- the hex value
* Returns : the decimal value delete the 0x
*********************************************************************************************************************************/
static int DeleteHex0x(unsigned char ucSource)
{

	int iReturn = 0;
	int iTempTen = (int)(ucSource >> 4);
	int iTempBite = (int)(ucSource & 0x0F);
	iReturn = iTempTen * 10 + iTempBite;
	return iReturn;

}

/*********************************************************************************************************************************
* Function Name : GetSatelliteInformation
* Description : Get the satellite information
* Parameters :
* ucDescriptor -- the descriptor buffer
* iDescriprotLength -- the descriptor length
* Returns : void
*********************************************************************************************************************************/
static void GetSatelliteInformation(unsigned char *pucDescriptor, unsigned int uiDescriprotLength)
{

	double dFrequency = 0;
	double dSymbolRate = 0;
	double dOrbitalPosition = 0;
	int aiFrequency[4] = {0};
	int aiSymbolRate[4] = {0};
	int aiOrbitalPosition[2] = {0};
	int nDesPos = -1;
	SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR Descriptor = {0};

	if (0 == GetSatelliteDeliverySystemDescriptor(&Descriptor, pucDescriptor, uiDescriprotLength))
	{
		aiFrequency[0] = DeleteHex0x((unsigned char)(Descriptor.frequency >> 24));
		aiFrequency[1] = DeleteHex0x((unsigned char)((Descriptor.frequency & 0x00FFFFFF) >> 16));
		aiFrequency[2] = DeleteHex0x((unsigned char)((Descriptor.frequency & 0x0000FFFF) >> 8));
		aiFrequency[3] = DeleteHex0x((unsigned char)(Descriptor.frequency & 0x000000FF));
		dFrequency = aiFrequency[0] * 100 + aiFrequency[1] / 10.0 + aiFrequency[2] / 1000.0 + aiFrequency[3] / 100000.0;

		aiOrbitalPosition[0] = DeleteHex0x((unsigned char)(Descriptor.orbital_position >> 8));
		aiOrbitalPosition[1] = DeleteHex0x((unsigned char)(Descriptor.orbital_position & 0x00FF));
		dOrbitalPosition = aiOrbitalPosition[0] * 100 + aiOrbitalPosition[1] / 10.0;

		aiSymbolRate[0] = DeleteHex0x((unsigned char)(Descriptor.symbol_rate >> 20));
		aiSymbolRate[1] = DeleteHex0x((unsigned char)((Descriptor.symbol_rate & 0x00FFFFF) >> 12));
		aiSymbolRate[2] = DeleteHex0x((unsigned char)((Descriptor.symbol_rate & 0x0000FFF) >> 4));
		aiSymbolRate[3] = DeleteHex0x((unsigned char)(Descriptor.symbol_rate & 0x000000F));
		dSymbolRate = aiSymbolRate[0] * 100 + aiSymbolRate[1] / 10.0 + aiSymbolRate[2] / 1000.0 + (aiSymbolRate[3] / 10) / 100000.0;

		printf("the satellite_delevery_system_descriptor information is \n");
		printf("frequency: %.5f GHz,\n", dFrequency);
		printf("orbital_position: %.1f degree\n", dOrbitalPosition);
		printf("symbol_rate: %.4f symbol/s\n", dSymbolRate);
	}

}

/*********************************************************************************************************************************
* Function Name : GetTerrestrialInformation
* Description : Get the satellite information
* Parameters :
* ucDescriptor -- the descriptor buffer
* iDescriprotLength -- the descriptor length
* Returns : void
*********************************************************************************************************************************/
static void GetTerrestrialInformation(unsigned char *pucDescriptor, unsigned int uiDescriprotLength)
{

	double dFrequency = 0;
	double dSymbolRate = 0;
	double dOrbitalPosition = 0;
	int aiFrequency[4] = {0};
	int aiSymbolRate[4] = {0};
	int aiOrbitalPosition[2] = {0};
	TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR Descriptor = {0};

	if (0 == GetTerrestrialDeliverySystemDescriptor(&Descriptor, pucDescriptor, uiDescriprotLength))
	{

		aiFrequency[0] = DeleteHex0x((unsigned char)(Descriptor.centre_frequency>> 24));
		aiFrequency[1] = DeleteHex0x((unsigned char)((Descriptor.centre_frequency & 0x00FFFFFF) >> 16));
		aiFrequency[2] = DeleteHex0x((unsigned char)((Descriptor.centre_frequency & 0x0000FFFF) >> 8));
		aiFrequency[3] = DeleteHex0x((unsigned char)(Descriptor.centre_frequency & 0x000000FF));
		dFrequency = aiFrequency[0] * 100 + aiFrequency[1] / 10.0 + aiFrequency[2] / 1000.0 + aiFrequency[3] / 100000.0;

		printf("the satellite_delevery_system_descriptor information is \n");
		printf("frequency: %.5f GHz,\n", dFrequency);

	}

}
/*********************************************************************************************************************************
* Function Name : GetTheServiceListInformation
* Description : Get the information in the service_list_descriptor
* Parameters :
* ucDescriptor -- the descriptor buffer
* iDescriprotLength -- the descriptor length
* Returns : void
*********************************************************************************************************************************/
static void GetTheServiceListInformation(unsigned char *pucDescriptor, unsigned int uiDescriprotLength)
{

	SERVICE_LIST_DESCRIPTOR stServiceListDescriptor = {0};
	SERVICE_LIST *pServiceListTemp = NULL;

	if (0 == GetServiceListDescriptor(&stServiceListDescriptor, pucDescriptor, uiDescriprotLength))
	{
		pServiceListTemp = stServiceListDescriptor.pServieListHead;
		printf("the service_list_descriptor information is \n");
		while (NULL != pServiceListTemp)
		{
			printf("the service_id is %x\tthe service_type is %x\n", pServiceListTemp->service_id, pServiceListTemp->service_type);
			pServiceListTemp = pServiceListTemp->next;
		}
		FreeServerListLink(&(stServiceListDescriptor.pServieListHead));
		printf("\n");
	}

}

/*********************************************************************************************************************************
* Function Name : GetNetworkName
* Description : Get the network name information
* Parameters :
* ucDescriptor -- the descriptor buffer
* iDescriprotLength -- the descriptor length
* Returns : void
*********************************************************************************************************************************/
static void GetNetworkName(unsigned char *pucDescriptor, unsigned int uiDescriprotLength)
{

	NETWORK_NAME_DESCRIPTOR stNetworkNameDescriptor = {0};

	if (0 == GetNetworkNameDescriptor(&stNetworkNameDescriptor, pucDescriptor, uiDescriprotLength))
	{
		printf("the network name is %s\n", stNetworkNameDescriptor.network_name);
	}

}

/*********************************************************************************************************************************
* Function Name : GetNitInformation
* Description : Get the NIT information
* Parameters :
* pNIT -- the TS_NIT
* Returns : void
*********************************************************************************************************************************/
static void GetNitInformation(TS_NIT *pNIT)
{

	TS_NIT_TRANSPORT *pTransport = NULL;

	pTransport = pNIT->pNitTranHead;
	GetNetworkName(pNIT->network_descriptor, pNIT->network_descriptors_length);
	while (NULL != pTransport)
	{
		GetSatelliteInformation(pTransport->descriptor, pTransport->transport_descriptors_length);
		GetTerrestrialInformation(pTransport->descriptor, pTransport->transport_descriptors_length);
		GetTheServiceListInformation(pTransport->descriptor, pTransport->transport_descriptors_length);
		pTransport = pTransport->next;
	}

}

/*********************************************************************************************************************************
* Function Name : ParseFromNIT
* Description : parse the NIT table
* Parameters :
* p -- the buffer of section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromNIT(unsigned char *p, unsigned char* pVersionNumber)
{
	TS_NIT mTsNIT = {0};
	if (-1 == ParseNIT(&mTsNIT, p, pVersionNumber))
	{
		printf("Parse NIT failed.");
		return -1;
	}
	GetNitInformation(&mTsNIT);
	FreeTsNitTransport(&(mTsNIT.pNitTranHead));
	return 0;

}
