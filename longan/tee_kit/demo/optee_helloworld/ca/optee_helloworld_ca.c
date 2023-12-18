
#include <tee_client_api.h>

#include <stdio.h>
#include <string.h>

//UUID for helloworld ta
static const TEEC_UUID helloworld_UUID={
	0x12345678,
	0x4321,
	0x8765,
	{0x9b,0x74,0xf3,0xfc,0x35,0x7c,0x7c,0x61}
};

int main(int argc,char **argv)
{
	TEEC_Context ctx;
	TEEC_Result teecErr;
	TEEC_Session teecSession;    
	TEEC_Operation operation;
	
	printf("CA:init context\n");
	
	teecErr = TEEC_InitializeContext(NULL,&ctx);
	if(teecErr!=TEEC_SUCCESS)
		goto failInit;

	printf("CA:open session\n");
	teecErr = TEEC_OpenSession(&ctx,&teecSession,
		&helloworld_UUID, TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
	if(teecErr!=TEEC_SUCCESS)
		goto failOpen;
	
	printf("CA:allocate memory\n");
	TEEC_SharedMemory tee_params_p;
    tee_params_p.size = 20;
    tee_params_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(&ctx, &tee_params_p) != TEEC_SUCCESS)
        goto failOpen;
	
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_NONE,
                                TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &tee_params_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

	if(argc >= 2){
		memcpy(tee_params_p.buffer,argv[1],10);
		printf("CA:invoke command: hello %10.10s\n",argv[1]);
	}else{
		printf("CA:invoke command\n");
	}
	teecErr = TEEC_InvokeCommand(&teecSession,
                        0x210, &operation, NULL);

    TEEC_ReleaseSharedMemory(&tee_params_p);
failOpen:
	TEEC_FinalizeContext(&ctx);
failInit:
	printf("CA:finish with %d",teecErr);

	if(teecErr==TEEC_SUCCESS)
		return 0;
	else
		return -1;
}

