package org.mqstack.ffmpegjni;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;
/**
 * Created by gongjia on 2017/9/1.
 */
public class MainActivity extends Activity implements OnItemClickListener {

    FFmpegJni ffmpegJni = null;
    private ListView listView;
    private ArrayAdapter<String> adapter;
    private String[] texts;
    private String[] commands;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ffmpegJni = new FFmpegJni();
        listView = (ListView) findViewById(R.id.orderList);
        texts = getResources().getStringArray(R.array.texts);
        commands = getResources().getStringArray(R.array.commands);
        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_expandable_list_item_1, texts);
        listView.setAdapter(adapter);
        listView.setOnItemClickListener(this);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
                            long id) {
        String testCommand = commands[position];
        Log.e("ffmpeg", testCommand);
        long i = System.currentTimeMillis();

        if(position < 4){
            if (ffmpegJni.ffmpegRunCommand(testCommand) == 0) {
                Toast.makeText(this, "success", Toast.LENGTH_LONG).show();
                Log.e("FFmpegJni", "success");
                Log.e("FFmpegJni", "command time" + (System.currentTimeMillis() - i));

            } else {
                Toast.makeText(this, "Cut failed", Toast.LENGTH_LONG).show();
                Log.e("FFmpegJni", "Cut failed");
            }
        }else if(position == 5){
            if (ffmpegJni.v2v_repeat("/sdcard/gongjia/xx.mp4", "/sdcard/gongjia/v2v_repeat.mp4", 2) == 0) {
                Toast.makeText(this, "v2v_repeat success", Toast.LENGTH_LONG).show();
                Log.e("FFmpegJni", "v2v_repeat success");
                Log.e("FFmpegJni", "command time" + (System.currentTimeMillis() - i));

            } else {
                Toast.makeText(this, "v2v_repeat failed", Toast.LENGTH_LONG).show();
                Log.e("FFmpegJni", "v2v_repeat failed");
            }
        }else if(position == 6){
            if (ffmpegJni.v2v_timeback("/sdcard/gongjia/xx.mp4", "/sdcard/gongjia/v2v_timeback.mp4") == 0) {
                Toast.makeText(this, "v2v_timeback success", Toast.LENGTH_LONG).show();
                Log.e("FFmpegJni", "v2v_timeback success");
                Log.e("FFmpegJni", "command time" + (System.currentTimeMillis() - i));

            } else {
                Toast.makeText(this, "v2v_timeback failed", Toast.LENGTH_LONG).show();
                Log.e("FFmpegJni", "v2v_timeback failed");
            }
        }

    }

}
