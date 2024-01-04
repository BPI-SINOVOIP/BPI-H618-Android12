/*
************************************************************************************************************************
*                                          Boot rom 
*                                         Seucre Boot
*
*                             Copyright(C), 2006-2013, AllWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name   : SBrom_certif.h
*
* Author      : glhuang
*
* Version     : 0.0.1
*
* Date        : 2013.09.05
*
* Description :
* 
* Others      : None at present.
*
*
* History     :
*
*  <Author>        <time>       <version>      <description>
*
* glhuang       2013.09.05       0.0.1        build the file
*
************************************************************************************************************************
*/


#include "type_def.h"
#include <string.h>
#include "SBrom_Certif.h"



struct I_struct_orig_1_3{
	u8 I_1_vers[5];
	u8 I_2_serial[3];
	u8 I_3_OID[13];
};


s32 _skip_squence(u8 * p_saddr , u32 len, u8 ** p_new_addr)
{
	u32 skip_len = 0;

	if(p_saddr[0] == 0x30){
		skip_len = p_saddr[1];
		skip_len += 2;

		if((skip_len >= 0x80) || (skip_len >= len)){
			printf("PANIC : _skip_squence() : to long ,error\n");
			return -1;
		}else{
			printf("skip squence %d bytes\n",skip_len);
			* p_new_addr = p_saddr + skip_len;
		}

	}else{
		printf("PANIC : _skip_squence() : error token\n");
		return -1;
	}
	return skip_len;
}


void _extend_INT(u32 int_src,
						u8 * p_buff , 
						u32  buff_len ,
						u32 * real_len )
{

	memset(p_buff,0,buff_len);

	if(int_src < 0x80){
		p_buff[0] = int_src;
		*real_len = 1;
	}else if(int_src < 0x100){
		p_buff[0] = 0x81;
		p_buff[1] = int_src;
		*real_len = 2;
	}else if(int_src < 0x10000){
		p_buff[0] = 0x82;
		p_buff[1] = (int_src >> 8) & 0xffff;
		p_buff[2] = (int_src >>  0) & 0xffff;
		*real_len = 3;
	}else if(int_src < 0x1000000){
		p_buff[0] = 0x83;
		p_buff[1] = (int_src >> 16) & 0xffff;
		p_buff[2] = (int_src >> 8) & 0xffff;
		p_buff[3] = (int_src >>  0) & 0xffff;
		*real_len = 4;
	}else{

		printf("PANIC : _extend_INT() : int too big\n");
	}

}
u32 _parser_INT(u8 * p_saddr ,
						u8 * p_buff , 
						u32 * p_data_len ,
						u32 * p_skip_len,
						u32 buff_len )
{
	u8 * p_now = p_saddr ;
	u32 para_len = 0;
	u32 has_zero = 0;
	u32 int_width_B_nr = 0;

	if(p_buff != NULL){
		memset(p_buff,0,buff_len);
	}

	if((p_now[0] != 0x02) && (p_now[0] != 0x30)){
	//	printf("PANIC : _parser_INT -0- token error\n");
	}

	if(((p_now[1]) & 0x80) == 0){
		para_len = p_now[1];
		*p_skip_len = 2 + para_len;
		//--hgl--FIXME--check
		if(((para_len & 0x01) ? 1 : 0) && ((p_now[2]) == 0) ){
			has_zero == 1;
			para_len --;
		}
		*p_data_len = para_len ;

		p_now += 2;
		if( has_zero ){
			if(((p_now[0]) != 0) || (!((p_now[1]) & 0x80))){
				printf("PANIC : _parser_INT -1- error\n");
			}
			p_now ++;
		}

		int_width_B_nr  = 1;
	}else if(p_now[1] == 0x81){

		para_len = p_now[2];
		*p_skip_len = 3 + para_len;

		if(((para_len & 0x01) ? 1 : 0) && ((p_now[3]) == 0) ){
			has_zero == 1;
			para_len --;
		}

		*p_data_len  = para_len;

		p_now += 3;
		if( has_zero ){
			if(((p_now[0]) != 0) || (!((p_now[1]) & 0x80))){
				printf("PANIC : _parser_INT -2- error\n");
			}
			p_now ++;
		}

		
		//sbrom_memcpy(p_buff,p_now,para_len);
		int_width_B_nr  = 2;
	}else if(p_now[1] == 0x82){


		para_len = ((p_now[2]) << 8) | (p_now[3]);
		*p_skip_len = 4 + para_len;

		if((para_len & 0x01) ? 1 : 0){
			has_zero == 1;
			para_len --;
		}

		*p_data_len = para_len;

		p_now += 4;
		if( has_zero ){
			if(((p_now[0]) != 0) || (!((p_now[1]) & 0x80))){
				printf("PANIC : _parser_INT -3- error\n");
			}
			p_now ++;
		}

		//sbrom_memcpy(p_buff,p_now,para_len);
		int_width_B_nr  = 3;
	}else{
		printf("PANIC : _parser_INT -4- error\n");
	}

	if(p_buff != NULL){
		if(para_len > buff_len){
			printf("PANIC : _parser_INT -5- overflow error\n");
			para_len = buff_len;
		}

		memcpy(p_buff,p_now,para_len);
	}
	return int_width_B_nr;
}
s32 _parser_I(u8 * p_saddr , u32 len ,struct x509_SBROMSW_certif* p_x509_SBROMSW)
{
	struct I_struct_orig_1_3 * p_I_1_3 = NULL;
	u8 * p_now = p_saddr;
	u8 * p_new = p_saddr;
	u32 skip_len = 0;
	u32 temp = 0;

	//--1~3,I_struct_orig_1_3{}
	if(p_now[0] != 0xa0){
		printf("PANIC : _parser_I() : version != 0xa0\n");
	}

#if	0
	// 1~3,I_struct_orig_1_3{}
	p_I_1_3 = (struct I_struct_orig_1_3 *)p_now;
	p_now += sizeof(*p_I_1_3);
	p_x509_SBROMSW->I_1_certif_ver = p_I_1_3->I_1_vers[4];
	p_x509_SBROMSW->I_2_certif_serial = p_I_1_3->I_2_serial[2];
	p_x509_SBROMSW->I_3_OID_len = (p_I_1_3->I_3_OID)[3];
	sbrom_memcpy(p_x509_SBROMSW->I_3_certif_algorithm_OID,
				&((p_I_1_3->I_3_OID)[4]),
				p_I_1_3->I_3_OID[3]);
#else
	p_now ++;
	p_now ++;

	_parser_INT(p_now,
				(u8 *)&(p_x509_SBROMSW->I_1_certif_ver),
				&temp,
				&skip_len ,
				sizeof(p_x509_SBROMSW->I_1_certif_ver));

	p_now += skip_len;

	_parser_INT(p_now,
				(u8 *)&(p_x509_SBROMSW->I_2_certif_serial),
				&temp,
				&skip_len ,
				sizeof(p_x509_SBROMSW->I_2_certif_serial));

	p_now += skip_len;


	if(p_now[0] != 0x30){
		printf("PANIC : _parser_I() : oid error\n");
	}
	p_x509_SBROMSW->I_3_OID_len = p_now[1];
	memcpy(p_x509_SBROMSW->I_3_certif_algorithm_OID,
				&(p_now[1]),
				p_x509_SBROMSW->I_3_OID_len);
	p_now +=  p_now[1];
	p_now += 2;
#endif

	//--4~6,skip squeuce
	p_new = p_now;
	skip_len = 0;

	skip_len += _skip_squence(p_new , len-skip_len, &p_new);
	skip_len += _skip_squence(p_new , len-skip_len, &p_new);
	skip_len += _skip_squence(p_new , len-skip_len, &p_new);
	p_now = p_new;

	//--7,人工根据版本
	if(p_now[0] != 0x30 || p_now[1] != 0x82){
		printf("PANIC : I_7 error\n");
	}
	p_now += 4;	//I_7 squence
	//OID
	memcpy(p_x509_SBROMSW->I_7_PK_algorithm_OID,
				&(p_now[2]),
				p_now[1]);
	p_now += p_now[1];
	p_now += 2;
	
	//
	if(p_now[0] != 0x30){
		printf("PANIC : I_7 PK error\n");
	}
	if((!(p_now[1] & 0x80))&& (((p_now[1]) & 0x7f) > 2 )){
		printf("PANIC : I_7 PK -1- error\n");
	}
	/*
	p_now += (p_now[1] & 0x7f);
	p_now += 2;
	p_now += 1;
	*/
	skip_len = _parser_INT(p_now ,
				NULL,
				&temp,
				&temp,
				0);
	p_now += skip_len;
	p_now ++;
	
	if(p_now[0] != 0x02){
		printf("PANIC : I_7 PK -3- error\n");
	}

	//m
	_parser_INT(p_now ,
				p_x509_SBROMSW->I_7_PK_para_m,
				&(p_x509_SBROMSW->I_7_PK_para_m_len),
				&skip_len,
				sizeof(p_x509_SBROMSW->I_7_PK_para_m));
	p_now += skip_len;
	
	//e
	_parser_INT(p_now ,
				p_x509_SBROMSW->I_7_PK_para_e,
				&(p_x509_SBROMSW->I_7_PK_para_e_len),
				&skip_len,
				sizeof(p_x509_SBROMSW->I_7_PK_para_e));
	p_now += skip_len;
	
	return (p_now - p_saddr);
}

s32 _parser_II(u8 * p_saddr , u32 len ,struct x509_SBROMSW_certif* p_x509_SBROMSW)
{
	u8 * p_now = p_saddr ;

	if(p_now[0] != 0xa3){
		printf("PANIC : _parser_II() : != 0xa3 error\n");
	}

	p_now += 2;
	p_now += 2;
	p_now += 2;
	memcpy(p_x509_SBROMSW->II_2_sha256_hash,
				p_now,
				32);
	p_now += 32;

	return p_now - p_saddr;
}			


s32 _parser_III(u8 * p_saddr , u32 len ,struct x509_SBROMSW_certif* p_x509_SBROMSW)
{
	u32 sign_len = 0;
	u8 * p_now = p_saddr ;
	u32 skip_len = 0;
	u32 temp = 0;

	skip_len = _parser_INT(p_now ,
				NULL,
				&temp,
				&temp,
				0);
	p_now += skip_len;
	p_now ++;
	
	//--1--oid
	if(p_now[0] != 0x30){
		printf("PANIC : _parser_III() oid error\n");
		return -1;
	}
	memcpy(p_x509_SBROMSW->III_1_OID,
					&(p_now[2]),
					p_now[1]);
	p_now += p_now[1];
	p_now += 2;

	//--2--sign
	if(p_now[0] != 0x03){
		printf("PANIC : _parser_III() sign error\n");
		return -1;
	}

	_parser_INT(p_now,
				p_x509_SBROMSW->III_2_sign_value,
				&(p_x509_SBROMSW->III_2_sign_len),
				&skip_len,
				sizeof(p_x509_SBROMSW->III_2_sign_value));
	p_now += skip_len;
				
	if(p_now - p_saddr != len){
		printf("PANIC : _parser_III() : last error\n");
	}
	return 0;
}	

static s32 _x509_parser(struct x509_SBROMSW_certif* p_x509_SBROMSW, u8 * p_buff_input , u32 buff_len)
{
	u32 total_len 	= 0;
	
	u8 * I_saddr 	= 0;	
	u32 I_len 		= 0;	
	u8 *  II_saddr 	= 0;
	u32 II_len 		= 0;
	u8 *  III_saddr = 0;
	u32 III_len 	= 0;
	u8 * p_now 		= p_buff_input;
	s32 temp = 0;


	total_len 	= 	((p_now[2]) << 0x08) | (p_now[3]);
	//--I--
	p_now 		+= 	4;
	I_len 		=	((p_now[2]) << 0x08) | (p_now[3]);
	p_x509_SBROMSW->misc_text_saddr = (u32)p_now;
	p_x509_SBROMSW->misc_text_len = I_len;
	p_now		+= 	4;
	I_saddr 	= 	p_now;

	p_now 		+=	I_len;

	//--III--
	if(p_now[0] != 0x03){	
		printf("III start token error token = 0x%x != 0x03,equence \n",p_now[0]);
		return -1;		
	}

	III_len 	= 	total_len - I_len - 4;
	III_saddr 	=	p_now;

	//check


	//
	temp = _parser_I(I_saddr,I_len,p_x509_SBROMSW);
	
	II_saddr = I_saddr + temp;
	II_len = I_len - temp;
	_parser_II(II_saddr , II_len , p_x509_SBROMSW);
	
	_parser_III(III_saddr,III_len, p_x509_SBROMSW);

	return 0;
};


s32  certif_open(u8 * p_addr , u32 len ,struct x509_total_info * p_info)
{
	s32 ret = 0;
	u32 temp = 0;

	
	memset(p_info,0,sizeof(struct x509_total_info));
	
	temp = (u32)p_addr;

	//check align

	
	p_info->p_addr = p_addr;
	p_info->certif_len = len ;

	ret = _x509_parser(&(p_info->certif_info), 
					p_info->p_addr,
					p_info->certif_len);
	
	p_info->is_inited = 1;

	return ret;
	
}

s32 certif_close(struct x509_total_info * p_info)
{
	p_info->is_inited = 0;
	memset(p_info,0,sizeof(*p_info));
	return 0;
}

s32 certif_get_hash(struct x509_total_info * p_info,
					u8 * p_buff , 
					u32 buff_len )
{
	u32 hash_len = sizeof(p_info->certif_info.II_2_sha256_hash);
	
	if(buff_len < hash_len){
		printf("PANIC : certif_get_hash() len error\n");
		return -1;
	}

	memcpy(p_buff,p_info->certif_info.II_2_sha256_hash,hash_len);
	
	return 0;
}
