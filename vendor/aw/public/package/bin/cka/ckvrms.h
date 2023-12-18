/*
  C-Kermit C RMS NAM[L]-related macros.
*/

#ifndef CKVRMS_H
#define CKVRMS_H

#include <rms.h>

#ifdef NAML$C_BID

/* Use long name (NAML) structure. */

#  define FAB_OR_NAML( fab, nam) nam
#  define FAB_OR_NAML_DNA naml$l_long_defname
#  define FAB_OR_NAML_DNS naml$l_long_defname_size
#  define FAB_OR_NAML_FNA naml$l_long_filename
#  define FAB_OR_NAML_FNS naml$l_long_filename_size

#  define FAB_L_NAMX fab$l_naml
#  define NAMX NAML
#  define NAMX_C_MAXRSS NAML$C_MAXRSS
#  define NAMX_M_NOCONCEAL NAML$M_NOCONCEAL
#  define NAMX_M_SEARCH_LIST NAML$M_SEARCH_LIST
#  define NAMX_M_SRCHXABS NAML$M_SRCHXABS
#  define NAMX_V_NODE naml$v_node
#  define NAMX_W_DID naml$w_did
#  define NAMX_W_DID_NUM naml$w_did_num
#  define NAMX_W_DID_SEQ naml$w_did_seq
#  define NAMX_T_DVI naml$t_dvi
#  define NAMX_B_DEV naml$l_long_dev_size
#  define NAMX_L_DEV naml$l_long_dev
#  define NAMX_B_DIR naml$l_long_dir_size
#  define NAMX_L_DIR naml$l_long_dir
#  define NAMX_L_ESA naml$l_long_expand
#  define NAMX_B_ESL naml$l_long_expand_size
#  define NAMX_B_ESS naml$l_long_expand_alloc
#  define NAMX_W_FID naml$w_fid
#  define NAMX_L_FNB naml$l_fnb
#  define NAMX_B_NAME naml$l_long_name_size
#  define NAMX_L_NAME naml$l_long_name
#  define NAMX_B_NODE naml$l_long_node_size
#  define NAMX_L_NODE naml$l_long_node
#  define NAMX_B_NOP naml$b_nop
#  define NAMX_L_RLF naml$l_rlf
#  define NAMX_L_RSA naml$l_long_result
#  define NAMX_B_RSL naml$l_long_result_size
#  define NAMX_B_RSS naml$l_long_result_alloc
#  define NAMX_B_TYPE naml$l_long_type_size
#  define NAMX_L_TYPE naml$l_long_type
#  define NAMX_B_VER naml$l_long_ver_size
#  define NAMX_L_VER naml$l_long_ver
#  define CC_RMS_NAMX cc$rms_naml

#else /* def NAML$C_BID */

/* Use short name (NAM) structure. */

#  define FAB_OR_NAML( fab, nam) fab
#  define FAB_OR_NAML_DNA fab$l_dna
#  define FAB_OR_NAML_DNS fab$b_dns
#  define FAB_OR_NAML_FNA fab$l_fna
#  define FAB_OR_NAML_FNS fab$b_fns

#  define FAB_L_NAMX fab$l_nam
#  define NAMX NAM
#  define NAMX_C_MAXRSS NAM$C_MAXRSS
#  define NAMX_M_NOCONCEAL NAM$M_NOCONCEAL
#  define NAMX_M_SEARCH_LIST NAM$M_SEARCH_LIST
#  define NAMX_M_SRCHXABS NAM$M_SRCHXABS
#  define NAMX_T_DVI nam$t_dvi
#  define NAMX_V_NODE nam$v_node
#  define NAMX_W_DID nam$w_did
#  define NAMX_W_DID_NUM nam$w_did_num
#  define NAMX_W_DID_SEQ nam$w_did_seq
#  define NAMX_B_DEV nam$b_dev
#  define NAMX_L_DEV nam$l_dev
#  define NAMX_B_DIR nam$b_dir
#  define NAMX_L_DIR nam$l_dir
#  define NAMX_L_ESA nam$l_esa
#  define NAMX_B_ESL nam$b_esl
#  define NAMX_B_ESS nam$b_ess
#  define NAMX_W_FID nam$w_fid
#  define NAMX_L_FNB nam$l_fnb
#  define NAMX_B_NAME nam$b_name
#  define NAMX_L_NAME nam$l_name
#  define NAMX_B_NODE nam$b_node
#  define NAMX_L_NODE nam$l_node
#  define NAMX_B_NOP nam$b_nop
#  define NAMX_L_RLF nam$l_rlf
#  define NAMX_L_RSA nam$l_rsa
#  define NAMX_B_RSL nam$b_rsl
#  define NAMX_B_RSS nam$b_rss
#  define NAMX_B_TYPE nam$b_type
#  define NAMX_L_TYPE nam$l_type
#  define NAMX_B_VER nam$b_ver
#  define NAMX_L_VER nam$l_ver
#  define CC_RMS_NAMX cc$rms_nam

#endif /* def NAML$C_BID [else] */

#endif /* ndef CKVRMS_H */
