package com.android.action;

/**
 * Created by sunming on 2016/12/27.
 */
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.TextView;

public class SecondActivity extends Activity {
    private TextView tv;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //设置当前的Activity的界面布局
        setContentView(R.layout.second);
        //获得Intent
        Intent intent = this.getIntent();
        tv = (TextView)findViewById(R.id.TextView1);
        //从Intent获得额外信息，设置为TextView的文本
        tv.setText(intent.getStringExtra("useName"));
    }
}

