#ifndef DC_WRAPPER_DTMBIP_H
#define DC_WRAPPER_DTMBIP_H

#ifdef __cpluscplus
extern "C"
{
#endif

/*
 * only one device, we need singleton instance 
 * and shold only has one owner 
 */
DC_DemodT *DTMBIP_SingletonGetInstance(void);

#ifdef __cpluscplus
}
#endif

#endif