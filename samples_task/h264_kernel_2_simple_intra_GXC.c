#include <stdint.h>
// #include "libavutil/common.h"
// #include "libavutil/intreadwrite.h"

#include <libavutil/common.h>
#include <libavutil/intreadwrite.h>

int pixel_shift = 0;                      ///< 0 for hl_decode_mb_simple_8, 1 for hl_decode_mb_simple_16
int bit_depth_luma = 8;                   ///< 亮度分量的位深度，可以取8、9、10、12、14
const uint32_t scan8[16 * 3 + 3] = {
    4 +  1 * 8, 5 +  1 * 8, 4 +  2 * 8, 5 +  2 * 8,
    6 +  1 * 8, 7 +  1 * 8, 6 +  2 * 8, 7 +  2 * 8,
    4 +  3 * 8, 5 +  3 * 8, 4 +  4 * 8, 5 +  4 * 8,
    6 +  3 * 8, 7 +  3 * 8, 6 +  4 * 8, 7 +  4 * 8,
    4 +  6 * 8, 5 +  6 * 8, 4 +  7 * 8, 5 +  7 * 8,
    6 +  6 * 8, 7 +  6 * 8, 6 +  7 * 8, 7 +  7 * 8,
    4 +  8 * 8, 5 +  8 * 8, 4 +  9 * 8, 5 +  9 * 8,
    6 +  8 * 8, 7 +  8 * 8, 6 +  9 * 8, 7 +  9 * 8,
    4 + 11 * 8, 5 + 11 * 8, 4 + 12 * 8, 5 + 12 * 8,
    6 + 11 * 8, 7 + 11 * 8, 6 + 12 * 8, 7 + 12 * 8,
    4 + 13 * 8, 5 + 13 * 8, 4 + 14 * 8, 5 + 14 * 8,
    6 + 13 * 8, 7 + 13 * 8, 6 + 14 * 8, 7 + 14 * 8,
    0 +  0 * 8, 0 +  5 * 8, 0 + 10 * 8
};

#define SRC(x,y) src[(x)+(y)*stride]
#define PT(x) \
    const int t##x = (SRC(x-1,-1) + 2*SRC(x,-1) + SRC(x+1,-1) + 2) >> 2;
#define PREDICT_8x8_DC(v) \
    int y; \
    for( y = 0; y < 8; y++ ) { \
        AV_WNA(32, ((uint32_t*)src)+0, v); \
        AV_WNA(32, ((uint32_t*)src)+1, v); \
        src += stride; \
    }

//可能作为变量输入
const int mb_x; //0-119
const int mb_y; //0-67
const int mb_xy; //mb_xy = mb_x + mb_y * 120
int mb_type = 0x0001;
//int mb_type = 0x0100;
long linesize = 5760;
long uvlinesize = 2880;
int qscale;
int pic_f_linesize_0;
int pic_f_linesize_1;
uint32_t non_zero_count_cache[120];
int8_t intra4x4_pred_mode_cache[40];
int topleft_samples_available  = 0xB3FF;
int top_samples_available      = 0x33FF;
int topright_samples_available = 0x26EA;
int16_t __attribute__ ((aligned (16))) mb[1536];



#define DC_PRED8x8             0
#define HOR_PRED8x8            1
#define VERT_PRED8x8           2
#define PLANE_PRED8x8          3
#define PIXEL_SPLAT_X4(x) ((x)*0x01010101U)
#define av_clip_pixel(a) av_clip_uint8(a)
int chroma_pred_mode = VERT_PRED8x8;
//int chroma_pred_mode = HOR_PRED8x8;
int chroma_format_idc = 1;
//int chroma_format_idc = 2;
//int chroma_format_idc = 3;
int chroma_y_shift = 1;

uint32_t *data[8];


#define IS_INTRA(a)      ((a) & 7)
#define IS_INTRA4x4(a) ((a) & 0x0001)
#define IS_8x8DCT(a)       ((a) & 0x01000000)
#define SIMPLE 1


int deblocking_filter;          ///< disable_deblocking_filter_idc with 1 <-> 0





av_always_inline void pred8x8_vertical_8_c(uint32_t *_src, long _stride)
{
    int i;
    uint32_t *src = (uint32_t*)_src;
    int stride = _stride>>(sizeof(uint32_t)-1);
    const uint32_t a= AV_RN32A(((uint32_t*)(src-stride))+0);
    const uint32_t b= AV_RN32A(((uint32_t*)(src-stride))+1);

    for(i=0; i<8; i++){
        AV_WN32A(((uint32_t*)(src+i*stride))+0, a);
        AV_WN32A(((uint32_t*)(src+i*stride))+1, b);
    }
}


av_always_inline void pred8x8_horizontal_8_c(uint32_t *_src, long stride)
{
    int i;
    uint32_t *src = (uint32_t*)_src;
    stride >>= sizeof(uint32_t)-1;

    for(i=0; i<8; i++){
        const uint32_t a = (src[-1+i*stride])*0x01010101U;
        AV_WN32A(((uint32_t*)(src+i*stride))+0, a);
        AV_WN32A(((uint32_t*)(src+i*stride))+1, a);
    }
}




av_always_inline void h264_idct8_dc_add(uint32_t *_dst, int16_t *_block, int stride){
    int i, j;
    uint32_t *dst = (uint32_t*)_dst;
    int16_t *block = (int16_t*)_block;
    int dc = (block[0] + 32) >> 6;
    block[0] = 0;
    stride /= sizeof(uint32_t);
    for( j = 0; j < 8; j++ )
    {
        for( i = 0; i < 8; i++ )
            dst[i] = av_clip_pixel( dst[i] + dc );
        dst += stride;
    }
}

av_always_inline void h264_idct8_add(uint32_t *_dst, int16_t *_block, int stride){
    int i;
    uint32_t *dst = (uint32_t*)_dst;
    int16_t *block = (int16_t*)_block;
    stride >>= sizeof(uint32_t)-1;

    block[0] += 32;

    for( i = 0; i < 8; i++ )
    {
        const int a0 =  block[i+0*8] + block[i+4*8];
        const int a2 =  block[i+0*8] - block[i+4*8];
        const int a4 = (block[i+2*8]>>1) - block[i+6*8];
        const int a6 = (block[i+6*8]>>1) + block[i+2*8];

        const int b0 = a0 + a6;
        const int b2 = a2 + a4;
        const int b4 = a2 - a4;
        const int b6 = a0 - a6;

        const int a1 = -block[i+3*8] + block[i+5*8] - block[i+7*8] - (block[i+7*8]>>1);
        const int a3 =  block[i+1*8] + block[i+7*8] - block[i+3*8] - (block[i+3*8]>>1);
        const int a5 = -block[i+1*8] + block[i+7*8] + block[i+5*8] + (block[i+5*8]>>1);
        const int a7 =  block[i+3*8] + block[i+5*8] + block[i+1*8] + (block[i+1*8]>>1);

        const int b1 = (a7>>2) + a1;
        const int b3 =  a3 + (a5>>2);
        const int b5 = (a3>>2) - a5;
        const int b7 =  a7 - (a1>>2);

        block[i+0*8] = b0 + b7;
        block[i+7*8] = b0 - b7;
        block[i+1*8] = b2 + b5;
        block[i+6*8] = b2 - b5;
        block[i+2*8] = b4 + b3;
        block[i+5*8] = b4 - b3;
        block[i+3*8] = b6 + b1;
        block[i+4*8] = b6 - b1;
    }
    for( i = 0; i < 8; i++ )
    {
        const int a0 =  block[0+i*8] + block[4+i*8];
        const int a2 =  block[0+i*8] - block[4+i*8];
        const int a4 = (block[2+i*8]>>1) - block[6+i*8];
        const int a6 = (block[6+i*8]>>1) + block[2+i*8];

        const int b0 = a0 + a6;
        const int b2 = a2 + a4;
        const int b4 = a2 - a4;
        const int b6 = a0 - a6;

        const int a1 = -block[3+i*8] + block[5+i*8] - block[7+i*8] - (block[7+i*8]>>1);
        const int a3 =  block[1+i*8] + block[7+i*8] - block[3+i*8] - (block[3+i*8]>>1);
        const int a5 = -block[1+i*8] + block[7+i*8] + block[5+i*8] + (block[5+i*8]>>1);
        const int a7 =  block[3+i*8] + block[5+i*8] + block[1+i*8] + (block[1+i*8]>>1);

        const int b1 = (a7>>2) + a1;
        const int b3 =  a3 + (a5>>2);
        const int b5 = (a3>>2) - a5;
        const int b7 =  a7 - (a1>>2);

        dst[i + 0*stride] = av_clip_pixel( dst[i + 0*stride] + ((b0 + b7) >> 6) );
        dst[i + 1*stride] = av_clip_pixel( dst[i + 1*stride] + ((b2 + b5) >> 6) );
        dst[i + 2*stride] = av_clip_pixel( dst[i + 2*stride] + ((b4 + b3) >> 6) );
        dst[i + 3*stride] = av_clip_pixel( dst[i + 3*stride] + ((b6 + b1) >> 6) );
        dst[i + 4*stride] = av_clip_pixel( dst[i + 4*stride] + ((b6 - b1) >> 6) );
        dst[i + 5*stride] = av_clip_pixel( dst[i + 5*stride] + ((b4 - b3) >> 6) );
        dst[i + 6*stride] = av_clip_pixel( dst[i + 6*stride] + ((b2 - b5) >> 6) );
        dst[i + 7*stride] = av_clip_pixel( dst[i + 7*stride] + ((b0 - b7) >> 6) );
    }

    memset(block, 0, 64 * sizeof(int16_t));
}

av_always_inline void pred8x8l_top_dc(uint32_t *_src, int has_topleft,
                                   int has_topright, ptrdiff_t _stride)
{
    uint32_t *src = (uint32_t*)_src;
    int stride = _stride>>(sizeof(uint32_t)-1);

    const int t0 = ((has_topleft ? SRC(-1,-1) : SRC(0,-1))  + 2*SRC(0,-1) + SRC(1,-1) + 2) >> 2; 
    PT(1) PT(2) PT(3) PT(4) PT(5) PT(6)
    const int t7 av_unused = ((has_topright ? SRC(8,-1) : SRC(7,-1)) + 2*SRC(7,-1) + SRC(6,-1) + 2) >> 2;
    const uint32_t dc = PIXEL_SPLAT_X4((t0+t1+t2+t3+t4+t5+t6+t7+4) >> 3);
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
    AV_WNA(32, ((uint32_t*)src)+0, dc); 
    AV_WNA(32, ((uint32_t*)src)+1, dc); 
    src += stride; 
}

av_always_inline int dctcoef_get(int16_t *mb, int high_bit_depth,
                                        int index)
{
        return AV_RN16A(mb + index);
}

av_always_inline void hl_decode_mb_predict_luma(int mb_type,
                                                       int pixel_shift,
                                                       const int *block_offset,
                                                       int linesize,
                                                       uint32_t *dest_y, int p)
{
    for (int i = 0; i < 16; i += 4) {
        uint32_t *const ptr = dest_y + block_offset[i];
        const int dir      = 10;
        const int nnz = non_zero_count_cache[scan8[i]];
        pred8x8l_top_dc(ptr, (topleft_samples_available << i) & 0x8000,
                                (topright_samples_available << i) & 0x4000, linesize);
            // if (nnz == 1 && dctcoef_get(mb, pixel_shift, i * 16 + p * 256))
                h264_idct8_dc_add(ptr, mb + (i * 16 ), linesize);
            // else
            //     h264_idct8_add(ptr, mb + (i * 16 + p * 256 << pixel_shift), linesize);

    }
}



void hl_decode_mb_simple_8() {

    const int block_h   = 16 >> chroma_y_shift;
    uint32_t *dest_y, *dest_cb, *dest_cr;
    int blockoffset[96];


    for (int i = 0; i < 16; i++) {
        blockoffset[i]           = 4 * ((scan8[i] - scan8[0]) & 7) + 4 * pic_f_linesize_0 * ((scan8[i] - scan8[0]) >> 3);
        blockoffset[48 + i]      = 4 * ((scan8[i] - scan8[0]) & 7) + 8 * pic_f_linesize_0 * ((scan8[i] - scan8[0]) >> 3);
    }    

    for (int i = 0; i < 16; i++) {
        blockoffset[16 + i]      =
        blockoffset[32 + i]      = 4 * ((scan8[i] - scan8[0]) & 7) + 4 * pic_f_linesize_1 * ((scan8[i] - scan8[0]) >> 3);
        blockoffset[48 + 16 + i] =
        blockoffset[48 + 32 + i] = 4 * ((scan8[i] - scan8[0]) & 7) + 8 * pic_f_linesize_1 * ((scan8[i] - scan8[0]) >> 3);
    }



    dest_cb = data[1] +  mb_x * 8 + mb_y * uvlinesize * block_h;
            //chroma_pred_mode = VERT_PRED8x8
            pred8x8_vertical_8_c(dest_cb, uvlinesize);

    dest_cr = data[2] +  mb_x * 8 + mb_y * uvlinesize * block_h;
            pred8x8_vertical_8_c(dest_cr, uvlinesize);



            //chroma_pred_mode = HOR_PRED8x8
            //pred8x8_horizontal_8_c(dest_cb, uvlinesize);
            //pred8x8_horizontal_8_c(dest_cr, uvlinesize);
loop_begin();
    dest_y = data[0] + (mb_x + mb_y * linesize) * 16;
            hl_decode_mb_predict_luma(mb_type, pixel_shift,
                                      blockoffset, linesize, dest_y, 0);
loop_end();

        //hl_decode_mb_idct_luma(mb_type, pixel_shift, blockoffset, linesize, dest_y, 0);

}
