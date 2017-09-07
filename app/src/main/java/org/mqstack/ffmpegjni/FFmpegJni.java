package org.mqstack.ffmpegjni;

import android.util.Log;

/**
 * Created by gongjia on 2017/9/1.
 */
public class FFmpegJni {

    static {
        System.loadLibrary("ijkffmpeg");
        System.loadLibrary("ffmpegjni");
    }

    public int ffmpegRunCommand(String command) {
        if (command.isEmpty()) {
            return 1;
        }
        String[] args = command.split(" ");
        for (int i = 0; i < args.length; i++) {
            Log.e("ffmpeg-jni", args[i]);

        }
        return run(args.length, args);
    }

    public native int run(int argc, String[] args);
    //反复特效
    public native int v2v_repeat(String inPath, String outPath, int arg);
    //时光倒流特效
    public native int v2v_timeback(String inPath, String outPath);

}
