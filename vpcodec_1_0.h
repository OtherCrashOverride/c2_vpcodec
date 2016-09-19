#ifndef _INCLUDED_COM_VIDEOPHONE_CODEC
#define _INCLUDED_COM_VIDEOPHONE_CODEC

#ifdef __cplusplus
extern "C" {
#endif

#define vl_codec_handle_t long

    typedef enum vl_codec_id_e {
        CODEC_ID_NONE,
        CODEC_ID_VP8,
        CODEC_ID_H261,
        CODEC_ID_H263,
        CODEC_ID_H264, /* Currently, suport H264 only */
        CODEC_ID_H265,

    } vl_codec_id_t;

    typedef enum vl_img_format_e
    {
        IMG_FMT_NONE,
        IMG_FMT_NV12,

    } vl_img_format_t;

    typedef enum vl_frame_type_e
    {
        FRAME_TYPE_NONE,
        FRAME_TYPE_AUTO,
        FRAME_TYPE_IDR,
        FRAME_TYPE_I,
        FRAME_TYPE_P,

    } vl_frame_type_t;

    /**
     * Get codec version info
     *
     *@return : codec version
     */
    const char *vl_get_version();

    /**
     * initialize encoder codec 
     *
     *@param : codec_id 
     *@param : width 
     *@param : height 
     *@param : frame_rate 
     *@param : bit_rate 
     *@param : gop GOP value
     *@param : img_format
     *@return : if success return encoder codec handle，else return <= 0
     */
    vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format);

    /**
     * encode image 
     *
     *@param : handle: encoder handle
     *@param : type: frame tpye 
     *@param : in: input data
     *@param : in_size: input data size (not used currently) 
     *@param : out: out put data
     *@return ：if success return the data lenght of out put data, else return <= 0
     */
    int vl_video_encoder_encode(vl_codec_handle_t handle, vl_frame_type_t type, char *in, int in_size, char **out);

    /**
     * destroy encoder
     *
     *@param ：handle:  encoder handle
     *@return ：if succes return 1，else 0
     */
    int vl_video_encoder_destory(vl_codec_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */
