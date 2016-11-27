package com.example.vapa.bubbles;

import android.content.Intent;
import android.content.res.AssetManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;

public class BubblesActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bubbles);

        Intent intent = getIntent();
        String endpoint_str = intent.getStringExtra(MainActivity.EXTRA_MESSAGE);
        String secure_str = intent.getStringExtra(MainActivity.EXTRA_SECURE);
        String auto_str = intent.getStringExtra(MainActivity.EXTRA_AUTO);
        String room_str = intent.getStringExtra(MainActivity.EXTRA_ROOM);

        boolean secure = secure_str.matches("secure");
        boolean auto_pilot = auto_str.matches("auto");

        String authority_verify_str = loadAssetFile("certs/bubbles-ca-cert.pem");
        String client_cert_str = loadAssetFile("certs/bubbles-client-cert.pem");
        String client_key_str = loadAssetFile("certs/bubbles-client-key.pem");

        //Log.i("BubblesActivity", "authority: " + authority_verify_str);
        //Log.i("BubblesActivity", "cert: " + client_cert_str);
        //Log.i("BubblesActivity", "key: " + client_key_str);

        if(nativeStart(endpoint_str, room_str, secure, auto_pilot, authority_verify_str, client_cert_str, client_key_str)){
            Log.i("BubblesActivity", "Success starting native engine");
        }else{
            Log.e("BubblesActivity", "Error starting native engine");
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        Log.i("BubblesActivity", "onResume");
        nativeResume();
    }

    @Override
    public void onPause () {
        super.onPause();
        Log.i("BubblesActivity", "onPause");
        nativePause();
    }

    private String loadAssetFile(String _file){
        AssetManager assetMgr = this.getAssets();
        ByteArrayOutputStream result = new ByteArrayOutputStream();
        try {
            InputStream inputStream = assetMgr.open(_file);

            byte[] buffer = new byte[1024];
            int length;
            while ((length = inputStream.read(buffer)) != -1) {
                result.write(buffer, 0, length);
            }
        }catch (IOException e){
            return new String("");
        }
        try{
            return result.toString("UTF-8");
        }catch(UnsupportedEncodingException e){
            return new String("");
        }

    }

    public native boolean nativeStart(String _endpoint, String _room, boolean _secure, boolean _auto_pilot, String _verify_authority, String _client_cert, String _client_key);
    public native boolean nativePause();
    public native boolean nativeResume();
}
