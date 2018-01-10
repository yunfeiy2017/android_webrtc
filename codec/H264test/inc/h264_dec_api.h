#ifndef __H264_DEC_API_H__
#define __H264_DEC_API_H__

typedef struct 
{
	unsigned char *InBuf;
	unsigned char *OutBuf;     
	unsigned int  InPutLen;
	unsigned int  InPutUsed;
	unsigned int  OutputLen;

}GVE_H264Dec_OperatePar;

typedef struct
{
	int temp;

}GVE_H264Dec_ConfigPar;

typedef struct 
{
	int width;
	int height;

}GVE_H264Dec_OutPutInfo;


int GVE_H264_Decoder_Create(unsigned int           *GVE_H264Dec_Handle,
							GVE_H264Dec_OperatePar *OperatePar,
							GVE_H264Dec_ConfigPar  *ConfigPar,
							GVE_H264Dec_OutPutInfo *OutPutInfo);

int GVE_H264_Decoder_Decoder(unsigned int            GVE_H264Dec_Handle,
							 GVE_H264Dec_OperatePar *OperatePar,
							 GVE_H264Dec_ConfigPar  *ConfigPar,
							 GVE_H264Dec_OutPutInfo *OutPutInfo); 

void GVE_H264_Decoder_Destroy(unsigned int           GVE_H264Dec_Handle);

#endif