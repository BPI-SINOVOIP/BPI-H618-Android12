#ifndef TS_TYPES_H
#define TS_TYPES_H
#include <CdxList.h>

#define PAT_PID 0x0000
#define SDT_PID 0x0011

#define PAT_TID 0x00
#define PMT_TID 0x02
#define SDT_TID 0x42

#define PROGRAM_AUDIO_MAX 4
#define PROGRAM_NAME_LENGTH 32
#define PROGRAM_MULTI_LANGUAGE_LENTH 3
#define PROGRAM_MULTI_NAME_LIST_LENTH   8

#define PMT_STREAM_TYPE_MPEG1_P2_V      0x01
#define PMT_STREAM_TYPE_MPEG2_P2_V      0x02
#define PMT_STREAM_TYPE_MPEG1_P3_A      0x03
#define PMT_STREAM_TYPE_MPEG2_P3_A_MP3  0x04

#define PMT_STREAM_TYPE_MPEG2_P1_PRI    0x06

#define PMT_STREAM_TYPE_MPEG2_P7_A_AAC  0x0f
#define PMT_STREAM_TYPE_MPEG4_P2_V      0x10
#define PMT_STREAM_TYPE_MPEG4_P3_A      0x11

#define PMT_STREAM_TYPE_MPEG_P10_V_AVC  0x1b
#define PMT_STREAM_TYPE_CAVS            0x42

#define PMT_STREAM_TYPE_MPEG2_P2_V_DC   0x80 /* DigiCipher II */

#define PMT_STREAM_TYPE_A_AC3           0x81
#define PMT_STREAM_TYPE_A_EAC3          0x87
#define PMT_STREAM_TYPE_V_VC1           0xea

typedef enum TSPackageTypeE TSPackageTypeT;
enum TSPackageTypeE
{
    TSPT_PAT,
    TSPT_PMT,
};

typedef enum TSStreamTypeE TSStreamTypeT;
enum TSStreamTypeE
{
    TS_STREAM_TYPE_UNKNOW,
    TS_STREAM_TYPE_AUDIO,
    TS_STREAM_TYPE_VIDEO,
};
#pragma pack(4)

struct PAT_ProgramInfoS
{
    uint32_t program_number:16;
    uint32_t reserved:3;
    union 
    {
    uint32_t network_PID:13;        /* program_number == 0x0000 */
    uint32_t program_map_PID:13;    /* program_number != 0x0000 */
    };
    CdxListNodeT node;
};

struct TS_HeaderS /* 4 Byte */
{
    uint32_t sync_byte:8;
    uint32_t transport_error_indicator:1;   /* 传输错误标志位，一般传输错误的话就不会处理这个包了 */
    uint32_t payload_unit_start_indicator:1;/* 有效负载的开始标志 */
    uint32_t transport_priority:1;          /* 传输优先级位，1表示高优先级 */
    uint32_t PID:13;                        /* 有效负载数据的类型 */
    uint32_t transport_scrambling_control:2;/* 加密标志位,00表示未加密 */
    uint32_t adaption_field_control:2;      /* 调整字段控制,。01仅含有效负载，10仅含调整字段，11含有调整字段和有效负载。为00的话解码器不进行处理。*/
    uint32_t continuity_counter:4;          /* 一个4bit的计数器，范围0-15 */
};
/* adaption field */
/* payload */

struct TS_AdaptionFieldS
{
    uint32_t adaption_field_length:8;
    uint32_t reserved:24; /* not parse adaption field now. */
};

struct PAT_S
{
    uint32_t table_id:8;
    uint32_t section_syntax_indicator:1;
    uint32_t zero:1;
    uint32_t reserved_1:2;
    uint32_t section_length:12;
    uint32_t transport_stream_id:16;
    uint32_t reserved_2:2;
    uint32_t version_number:5;
    uint32_t current_next_indicator:1;
    uint32_t section_number:8;
    uint32_t last_section_number:8;

    uint32_t program_cnt;
    CdxListT program_list;

    uint32_t CRC_32;

};

struct TS_DescriptorS
{
    uint32_t descriptor_tag:8;
    uint32_t descriptor_length:8;
    uint8_t data[64];

    CdxListNodeT node;
};

struct PMT_StreamS
{
    uint32_t stream_type :8;
    uint32_t reserved_1 :3;
    uint32_t elementary_PID :13;
    uint32_t reserved_2 :4;
    uint32_t ES_info_length :12;
    
    uint32_t descriptor_cnt;
    CdxListT descriptor_list;

    CdxListNodeT node;
};

struct PMT_S
{
    uint32_t table_id :8;
    uint32_t section_syntax_indicator :1;
    uint32_t zero :1;
    uint32_t reserved_1 :2;
    uint32_t section_length :12;
    uint32_t program_number :16;
    uint32_t reserved_2 :2;
    uint32_t version_number :5;
    uint32_t current_next_indicator :1;
    uint32_t section_number :8;
    uint32_t last_section_number :8;
    uint32_t reserved_3 :3;
    uint32_t PCR_PID :13;
    uint32_t reserved_4 :4;
    uint32_t program_info_length :12;

    uint32_t descriptor_cnt;
    CdxListT descriptor_list;

    uint32_t stream_cnt;
    CdxListT stream_list;

    uint32_t CRC_32 :32;    
};

struct SDT_ServiceS
{
	uint32_t service_id :16;
	uint32_t reserved_future_use :6;
	uint32_t EIT_schedule_flag :1;
	uint32_t EIT_present_following_flag :1; /* 0x0F */
	uint32_t running_status :3;
	uint32_t free_CA_mode :1;
	uint32_t descriptors_loop_length :12;
	
    uint32_t descriptor_cnt;
    CdxListT descriptor_list;

    CdxListNodeT node;
};

struct SDT_S
{
    uint32_t table_id :8;
    uint32_t section_syntax_indicator :1;
    uint32_t reserved_future_use_1 :1;
    uint32_t reserved_1 :2;
    uint32_t section_length :12;
    uint32_t transport_stream_id :16;
    uint32_t reserved_2 :2;
    uint32_t version_number :5;
    uint32_t current_next_indicator :1;
    uint32_t section_number :8;
    uint32_t last_section_number :8;
    uint32_t original_network_id :16;
    uint32_t reserved_future_use_2 :8;

    uint32_t service_cnt;
    CdxListT service_list;
    
    uint32_t CRC_32 :32;
};

struct MultiNameLoopS
{
    char multi_language[PROGRAM_MULTI_LANGUAGE_LENTH];
    char multi_name[PROGRAM_NAME_LENGTH];
};

struct ProgramInfoS
{
    uint32_t index;
    uint32_t valid;
	uint32_t pmt_pid;
    uint32_t video_pid;
    uint32_t video_codec_type;
    uint32_t audio_codec_type;
    uint32_t audio_num;
    uint32_t audio_pid[PROGRAM_AUDIO_MAX];
    uint32_t scrambled;
    uint32_t free_CA_mode;
    uint32_t running_status;
    uint32_t EIT_schedule_flag;
    uint32_t EIT_present_following_flag;
	char name[PROGRAM_NAME_LENGTH];
    int multi_name_loop_count;
    struct MultiNameLoopS multi_name_loop[PROGRAM_MULTI_NAME_LIST_LENTH];
};
#pragma pack(0)

#endif
