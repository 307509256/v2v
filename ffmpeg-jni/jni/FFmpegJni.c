/**
 * Created by gongjia on 2017/9/1.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <jni.h>
#include <unistd.h>

#include "logjni.h"
#include "ffmpeg.h"
#include "FFmpegJni.h"
#include "libavutil/ffversion.h"

#define JNI_CLASS_FFMPEG_API            "org/mqstack/ffmpegjni/FFmpegJni"

static JavaVM* g_jvm;

typedef struct ffmpeg_fields_t {
    pthread_mutex_t mutex;
    jclass clazz;
} ffmpeg_fields_t;

static ffmpeg_fields_t g_clazz;



static int jni_run_cmd(JNIEnv *env, jobject obj, jint argc, jobjectArray args) {

    int i = 0;
    char **argv = NULL;
    jstring *strr = NULL;
    LOGE("ffmpeg argc: %d", argc);

    pthread_mutex_lock(&g_clazz.mutex);

    if (args != NULL) {
       argv = (char **) malloc(sizeof(char *) * argc);
       strr = (jstring *) malloc(sizeof(jstring) * argc);

       for (i = 0; i < argc; ++i) {
           strr[i] = (jstring)(*env)->GetObjectArrayElement(env, args, i);
           argv[i] = (char *)(*env)->GetStringUTFChars(env, strr[i], 0);
           LOGE("ffmpeg args: %s", argv[i]);
       }
    }
    int result = -100;
    LOGE("Run ffmpeg");
    LOGE("ffmpeg av_version_info: %s",  (jstring)av_version_info());
    result = run_cmd((int)argc, argv);
    LOGE("ffmpeg result %d", result);

    for (i = 0; i < argc; ++i) {
       (*env)->ReleaseStringUTFChars(env, strr[i], argv[i]);
    }
    free(argv);
    free(strr);

    pthread_mutex_unlock(&g_clazz.mutex);

}
static jint jni_v2v_repeat(JNIEnv *env, jobject thiz, jstring inPath, jstring outPath, int arg)
{

	const char* psrcpath = (*env)->GetStringUTFChars(env, inPath, NULL );
	const char* pdstpath = (*env)->GetStringUTFChars(env, outPath, NULL );

	LOGE("[gdebug %s, %d] psrcpath= %p-%s\n",__FUNCTION__, __LINE__,psrcpath, psrcpath);
	LOGE("[gdebug %s, %d] pdstpath= %p-%s\n",__FUNCTION__, __LINE__,pdstpath, pdstpath);

	jint res = v2v_repeat(psrcpath, pdstpath, arg);

	LOGE("[gdebug %s, %d] res = %d\n",__FUNCTION__, __LINE__, res);
	(*env)->ReleaseStringUTFChars(env, inPath, psrcpath);
	(*env)->ReleaseStringUTFChars(env, outPath, pdstpath);

	LOGE("[gdebug %s, %d] res2 = %d\n",__FUNCTION__, __LINE__, res);
	return res;
}

static jint jni_v2v_timeback(JNIEnv *env, jobject thiz, jstring inPath, jstring outPath)
{

	const char* psrcpath = (*env)->GetStringUTFChars(env, inPath, NULL );
	const char* pdstpath = (*env)->GetStringUTFChars(env, outPath, NULL );

	LOGE("[gdebug %s, %d] psrcpath= %p-%s\n",__FUNCTION__, __LINE__,psrcpath, psrcpath);
	LOGE("[gdebug %s, %d] pdstpath= %p-%s\n",__FUNCTION__, __LINE__,pdstpath, pdstpath);

	jint res = v2v_timeback(psrcpath, pdstpath);

	LOGE("[gdebug %s, %d] res = %d\n",__FUNCTION__, __LINE__, res);
	(*env)->ReleaseStringUTFChars(env, inPath, psrcpath);
	(*env)->ReleaseStringUTFChars(env, outPath, pdstpath);

	LOGE("[gdebug %s, %d] res2 = %d\n",__FUNCTION__, __LINE__, res);
	return res;
}

static JNINativeMethod g_methods[] = {
    {"run", "(I[Ljava/lang/String;)I", (void *) jni_run_cmd},
    {"v2v_repeat","(Ljava/lang/String;Ljava/lang/String;I)I", (void *) jni_v2v_repeat},
    {"v2v_timeback","(Ljava/lang/String;Ljava/lang/String;)I", (void *) jni_v2v_timeback},
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv* env = NULL;

    g_jvm = vm;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    pthread_mutex_init(&g_clazz.mutex, NULL );

    // FindClass returns LocalReference
    g_clazz.clazz = (*env)->FindClass(env, JNI_CLASS_FFMPEG_API);
    if (g_clazz.clazz == NULL) {
        LOGE("[gdebug %s, %d] \n",__FUNCTION__, __LINE__);
        return -1;
    }
    int mothodcount = sizeof(g_methods)/sizeof(JNINativeMethod);
    (*env)->RegisterNatives(env, g_clazz.clazz, g_methods, mothodcount);
    LOGE("[gdebug %s, %d] \n",__FUNCTION__, __LINE__);
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    pthread_mutex_destroy(&g_clazz.mutex);
}
