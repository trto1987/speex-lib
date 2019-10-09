#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "speex/speex.h"
#include "speex/speex_preprocess.h"
#include "speex/speex_echo.h"

// SpeexBits bits;
void *dec_state;
SpeexPreprocessState *preprocess_state;
SpeexEchoState *echo_state;

int C_DENOISE = 1;
int C_NOISE_SUPPRESS = -25;
int frame_size;

/** 初始化
 * @param jenv
 * @param jcls
 */
JNIEXPORT jint JNICALL
Java_com_trto1987_speex_Speex_init(JNIEnv *jenv, jclass jcls)
{
    /* 初始化比特率结构体 */
    // speex_bits_init(&bits);

    /* 定义并初始化解码器状态 */
    dec_state = speex_decoder_init(&speex_nb_mode);
    if (speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &frame_size) != 0)
    {
        return 0;
    }

    /* 定义预处理及设置 */
    preprocess_state = speex_preprocess_state_init(frame_size, 8000);
    if (preprocess_state == NULL)
    {
        return 0;
    }

    if (speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &C_DENOISE) != 0)
    {
        return 0;
    }
    if (speex_preprocess_ctl(
        preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &C_NOISE_SUPPRESS) != 0)
    {
        return 0;
    }

    /* 回声消除 */
    echo_state = speex_echo_state_init(frame_size, 800);
    if (echo_state == NULL)
    {
        return 0;
    }
    if (speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state) != 0)
    {
        return 0;
    }

    return frame_size;
}

/** 解码
 * @param jenv
 * @param jcls
 * @param arr_in 输入数据
 * @param arr_out 输出数据
 * @param size 输入数据长度
 * @return 0 如果没有错误, 非0正值为jni错误，非0负值为其它错误
 */
JNIEXPORT jint JNICALL
Java_com_trto1987_speex_Speex_decode(
    JNIEnv *jenv, jobject jcls, 
    jbyteArray arr_in, jshortArray arr_out, jint size)
{
    jboolean is_copy;
    jbyte *buffer_in;
    short *buffer_out;
    SpeexBits bits;

    buffer_in = (*jenv)->GetByteArrayElements(jenv, arr_in, &is_copy);
    if ((*jenv)->ExceptionOccurred(jenv))
    {
        return 1;
    }

    buffer_out = (short *) calloc(sizeof(short), frame_size);
    if (buffer_out == NULL)
    {
        return -1;
    }

    /* 初始化比特率结构体 */
    speex_bits_init(&bits);

    /* 清空bit结构体，以便解码器处理后面的新一帧音频数据 */
    // speex_bits_reset(&bits);
    speex_bits_read_from(&bits, buffer_in, size);
    if (speex_decode_int(dec_state, &bits, buffer_out) != 0)
    {
        return -2;
    }
    
    speex_preprocess_run(preprocess_state, buffer_out);

    /* 赋值 */
    (*jenv)->SetShortArrayRegion(jenv, arr_out, 0, frame_size, buffer_out);
    if ((*jenv)->ExceptionOccurred(jenv))
    {
        return 2;
    }

    /* 释放内存 */ 
    (*jenv)->ReleaseByteArrayElements(jenv, arr_in, buffer_in, 0);
    if ((*jenv)->ExceptionOccurred(jenv))
    {
        return 3;
    }
    free(buffer_out);
    speex_bits_destroy(&bits);

    return 0;
}

/** 清理
 * @param jenv
 * @param jcls
 */
JNIEXPORT void JNICALL
Java_com_trto1987_speex_Speex_close(JNIEnv *jenv, jclass jcls)
{
    /* 销毁释放资源 */
    speex_echo_state_destroy(echo_state);
    speex_preprocess_state_destroy(preprocess_state);
    speex_decoder_destroy(dec_state);
    // speex_bits_destroy(&bits);
}
