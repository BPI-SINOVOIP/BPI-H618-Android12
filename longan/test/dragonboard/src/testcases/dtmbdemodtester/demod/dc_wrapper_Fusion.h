#ifndef DC_WRAPPER_FUSION_H
#define DC_WRAPPER_FUSION_H

#ifdef __cpluscplus
extern "C"
{
#endif

/*
 * only one device, we need singleton instance 
 * and shold only has one owner 
 */
DC_DemodT *Fusion_SingletonGetInstance(void);

#ifdef __cpluscplus
}
#endif

#endif

