//#define LOG_NDEBUG 0
#define LOG_TAG "GXVENC_PARSER"
//#include <utils/Log.h>

#include "gxvenclib_fast.h"
#include "enc_define.h"
#include "parser.h"

//For Intra4x4/Intra16x16/Inter16x16/Inter16x8/Inter8x16
#define get_mb_x(addr) *((unsigned char *)(addr+7))
#define get_mb_y(addr) *((unsigned char *)(addr+6))
#define get_mb_type(addr) *((unsigned char *)(addr+5))
#define get_mb_CPred(addr) *((unsigned char *)(addr+4))

#define get_mb_LPred_I4(addr, mode) \
    mode[0] = *((unsigned char *)(addr+12)) & 0xf; \
    mode[1] = (*((unsigned char *)(addr+12)) >> 4) & 0xf; \
    mode[2] = *((unsigned char *)(addr+13)) & 0xf; \
    mode[3] = (*((unsigned char *)(addr+13)) >> 4) & 0xf; \
    mode[4] = *((unsigned char *)(addr+14)) & 0xf; \
    mode[5] = (*((unsigned char *)(addr+14)) >> 4) & 0xf; \
    mode[6] = *((unsigned char *)(addr+15)) & 0xf; \
    mode[7] = (*((unsigned char *)(addr+15)) >> 4) & 0xf; \
    mode[8] = *((unsigned char *)(addr+0)) & 0xf; \
    mode[9] = (*((unsigned char *)(addr+0)) >> 4) & 0xf; \
    mode[10] = *((unsigned char *)(addr+1)) & 0xf; \
    mode[11] = (*((unsigned char *)(addr+1)) >> 4) & 0xf; \
    mode[12] = *((unsigned char *)(addr+2)) & 0xf; \
    mode[13] = (*((unsigned char *)(addr+2)) >> 4) & 0xf; \
    mode[14] = *((unsigned char *)(addr+3)) & 0xf; \
    mode[15] = (*((unsigned char *)(addr+3)) >> 4) & 0xf;

#define get_mb_mv_P16x16(addr, mv) \
    { \
        int k = 0; \
        while(k<16){ \
            mv[k].mvx = *((short *)(addr+0)); \
            mv[k].mvy = *((short *)(addr+2)); \
            k++; \
        } \
    }

#define get_mb_mv_P16x8(addr, mv) \
    { \
        int k = 0; \
        while(k<8){ \
            mv[k].mvx = *((short *)(addr+0)); \
            mv[k].mvy = *((short *)(addr+2)); \
            mv[k+8].mvx = *((short *)(addr+12)); \
            mv[k+8].mvy = *((short *)(addr+14)); \
            k++; \
        } \
    }

#define get_mb_mv_P8x16(addr, mv) \
    { \
        int k = 0; \
        while(k<4){ \
            mv[k].mvx = *((short *)(addr+0)); \
            mv[k].mvy = *((short *)(addr+2)); \
            mv[k+8].mvx = *((short *)(addr+0)); \
            mv[k+8].mvy = *((short *)(addr+2)); \
            mv[k+4].mvx = *((short *)(addr+12)); \
            mv[k+4].mvy = *((short *)(addr+14)); \
            mv[k+12].mvx = *((short *)(addr+12)); \
            mv[k+12].mvy = *((short *)(addr+14)); \
            k++; \
        } \
    }

#define get_mb_LPred_I16(addr) *((unsigned char *)(addr+12)) & 0xf
#define get_mb_quant(addr) *((unsigned char *)(addr+11))
#define get_mb_cbp(addr) *((unsigned char *)(addr+10))
#define get_mb_IntraSAD(addr) *((unsigned short *)(addr+8))
#define get_mb_InterSAD(addr) *((unsigned short *)(addr+22))
#define get_mb_bits(addr) *((unsigned short *)(addr+16))

#define get_mb_quant_ex(addr) *((unsigned char *)(addr+67))
#define get_mb_cbp_ex(addr) *((unsigned char *)(addr+66))
#define get_mb_IntraSAD_ex(addr) *((unsigned short *)(addr+64))
#define get_mb_InterSAD_ex(addr) *((unsigned short *)(addr+78))
#define get_mb_bits_ex(addr) *((unsigned short *)(addr+72))

#define get_mb_mv_P8x8(addr, mv) \
    { \
        int k = 0, j, offset; \
        while(k<8){ \
            j = k << 1; \
            offset = k << 3; \
            mv[j].mvx = *((short *)(addr+0+offset)); \
            mv[j].mvy = *((short *)(addr+2+offset)); \
            mv[j+1].mvx = *((short *)(addr+12+offset)); \
            mv[j+1].mvy = *((short *)(addr+14+offset)); \
            k++; \
        } \
    }

int Parser_DumpInfo(gx_fast_enc_drv_t* p)
{
    int x,y;
    mb_t* info = p->mb_info;
    unsigned char* cur_mb = p->dump_buf.addr;
    unsigned char* next_mb = cur_mb;
    for(y = 0; y < p->src.mb_height; y++){
        for(x = 0; x < p->src.mb_width; x++){
            cur_mb = next_mb;
            info->mbx = get_mb_x(cur_mb);
            info->mby = get_mb_y(cur_mb);
            if((x != info->mbx)||(y != info->mby)) {
                //ALOGE("parser mb poistion error: actual: %dx%d, info:%dx%d",x,y,info->mbx,info->mby);
                return -1;
            }
            info->mb_type = get_mb_type(cur_mb);
            if((info->mb_type == HENC_MB_Type_I4MB)
                || (info->mb_type == HENC_MB_Type_I16MB)){
                info->intra.CPred = get_mb_CPred(cur_mb);
                if(info->mb_type == HENC_MB_Type_I16MB) {
                    info->intra.LPred[0] = get_mb_LPred_I16(cur_mb);
                } else {
                    get_mb_LPred_I4(cur_mb, info->intra.LPred);
                }
                info->intra.sad = get_mb_IntraSAD(cur_mb);
                info->inter.sad = get_mb_InterSAD(cur_mb);
                info->quant= get_mb_quant(cur_mb);
                info->cbp = get_mb_cbp(cur_mb);
                info->bits = get_mb_bits(cur_mb);
                next_mb = cur_mb + 24;
                if(info->mb_type == HENC_MB_Type_I16MB) 
                    info->final_sad = info->intra.sad - p->i16_weight;
                else
                    info->final_sad = info->intra.sad - p->i4_weight;
            } else if((info->mb_type == HENC_MB_Type_P16x16)
                || (info->mb_type == HENC_MB_Type_P16x8)
                || (info->mb_type == HENC_MB_Type_P8x16)
                || (info->mb_type == HENC_MB_Type_PSKIP)){
                if((info->mb_type == HENC_MB_Type_P16x16)
                    || (info->mb_type == HENC_MB_Type_PSKIP)) {
                    get_mb_mv_P16x16(cur_mb, info->inter.mv);
                } else if(info->mb_type == HENC_MB_Type_P16x8) {
                    get_mb_mv_P16x8(cur_mb, info->inter.mv);
                } else {
                    get_mb_mv_P8x16(cur_mb, info->inter.mv);
                }
                info->intra.sad = get_mb_IntraSAD(cur_mb);
                info->inter.sad = get_mb_InterSAD(cur_mb);
                info->quant= get_mb_quant(cur_mb);
                info->cbp = get_mb_cbp(cur_mb);
                info->bits = get_mb_bits(cur_mb);
                next_mb = cur_mb + 24;
                info->final_sad = info->inter.sad - p->me_weight;
            } else if (info->mb_type == HENC_MB_Type_P8x8) {
                get_mb_mv_P8x8(cur_mb, info->inter.mv);
                info->intra.sad = get_mb_IntraSAD_ex(cur_mb);
                info->inter.sad = get_mb_InterSAD_ex(cur_mb);
                info->quant= get_mb_quant_ex(cur_mb);
                info->cbp = get_mb_cbp_ex(cur_mb);
                info->bits = get_mb_bits_ex(cur_mb);
                next_mb = cur_mb + 80;
                info->final_sad = info->inter.sad - p->me_weight;
            } else if (info->mb_type == HENC_MB_Type_AUTO) { // work around
                info->intra.sad = get_mb_IntraSAD(cur_mb);
                info->inter.sad = get_mb_InterSAD(cur_mb);
                info->quant= get_mb_quant(cur_mb);
                info->cbp = get_mb_cbp(cur_mb);
                info->bits = get_mb_bits(cur_mb);
                next_mb = cur_mb + 24;
                if(info->inter.sad < info->intra.sad) {
                    get_mb_mv_P16x16(cur_mb, info->inter.mv);
                    info->final_sad = info->inter.sad - p->me_weight;
                    //ALOGV("frame:%d, parser mb (%dx%d) type %d warning--set as inter 16x16, cur_mb:0x%x, inter sad:%d, intra sad:%d", 
                    //    p->total_encode_frame+1, x, y, info->mb_type, (ulong)(cur_mb-p->dump_buf.addr), info->inter.sad, info->intra.sad);
                    info->mb_type = HENC_MB_Type_P16x16;
                } else {
                    info->intra.CPred = get_mb_CPred(cur_mb);
                    info->intra.LPred[0] = get_mb_LPred_I16(cur_mb);
                    info->final_sad = info->intra.sad - p->i16_weight;
                    //ALOGV("frame:%d, parser mb (%dx%d) type %d warning--set as I16(mode %d), cur_mb:0x%x, inter sad:%d, intra sad:%d", 
                    //    p->total_encode_frame+1, x, y, info->mb_type, info->intra.LPred[0], (ulong)(cur_mb-p->dump_buf.addr), info->inter.sad, info->intra.sad);
	             info->mb_type == HENC_MB_Type_I16MB;
                }
            } else {
                //ALOGE("parser mb (%dx%d) type %d error, cur_mb:0x%x", x, y, info->mb_type, (ulong)(cur_mb-p->dump_buf.addr));
                return -1;
            }
            if (info->final_sad <0)
                info->final_sad = 0;
        }
    }
    return 0;
}

