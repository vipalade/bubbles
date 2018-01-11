package com.example.vapa.bubbles;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    public final static String EXTRA_MESSAGE = "com.example.vapa.bubbles.MESSAGE";
    public final static String EXTRA_SECURE = "com.example.vapa.bubbles.SECURE";
    public final static String EXTRA_COMPRESS = "com.example.vapa.bubbles.COMPRESS";
    public final static String EXTRA_AUTO = "com.example.vapa.bubbles.AUTO";
    public final static String EXTRA_ROOM = "com.example.vapa.bubbles.ROOM";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void startEcho(View view){
        Intent intent = new Intent(this, BubblesActivity.class);
        EditText editText = (EditText) findViewById(R.id.editText);
        EditText editText2 = (EditText) findViewById(R.id.editText2);
        CheckBox checkBox = (CheckBox) findViewById(R.id.checkBox);
        CheckBox checkBox1 = (CheckBox) findViewById(R.id.checkBox1);
        CheckBox checkBox2 = (CheckBox) findViewById(R.id.checkBox2);
        String   message_str = editText.getText().toString();
        String   secure_str;
        String   compress_str;
        String   auto_str;
        String   room_str = editText2.getText().toString();
        if(checkBox.isChecked())
            secure_str = "yes";
        else
            secure_str = "no";

        if(checkBox1.isChecked())
            compress_str = "yes";
        else
            compress_str = "no";

        if(checkBox2.isChecked())
            auto_str = "auto";
        else
            auto_str = "";

        intent.putExtra(EXTRA_MESSAGE, message_str);
        intent.putExtra(EXTRA_SECURE, secure_str);
        intent.putExtra(EXTRA_COMPRESS, compress_str);
        intent.putExtra(EXTRA_AUTO, auto_str);
        intent.putExtra(EXTRA_ROOM, room_str);
        startActivity(intent);
    }
}
