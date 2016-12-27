package com.android.action;

import android.content.DialogInterface;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.ButtonBarLayout;
import android.view.View;
import android.widget.Button;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;


public class MainActivity extends AppCompatActivity {
    private Button getBtn;
    private Button getBtn2;
    private Button btn;
    private EditText etx;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        getBtn=(Button)findViewById(R.id.getBtn);
        getBtn.setOnClickListener(new View.OnClickListener(){
            public void onClick(View v){
                Intent  intent = new Intent();
                intent.setAction(Intent.ACTION_GET_CONTENT);//设置Intent Action属性
             //   intent.setAction(Intent.ACTION_g);//设置Intent Action属性
//                intent.setType("vnd.android.cursor.item/phone");// 设置Intent Type 属性
                intent.setType("image/*;video/*;audio/*");// 设置Intent Type 属性


                //主要是获取通讯录的内容

                startActivity(intent); // 启动Activity
            }
        });

        getBtn2=(Button)findViewById(R.id.Button1);
        getBtn2.setOnClickListener(new View.OnClickListener(){
            public void onClick(View v){
                Intent  intent = new Intent();
                intent.setAction(Intent.ACTION_MAIN);// 添加Action属性
                intent.addCategory(Intent.CATEGORY_HOME);// 添加Category属性
                startActivity(intent);// 启动Activity
            }
        });

        btn = (Button)findViewById(R.id.Button2);
        etx = (EditText)findViewById(R.id.EditText1);

        btn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent();
                //设置Intent的class属性，跳转到SecondActivity
                intent.setClass(MainActivity.this, SecondActivity.class);
                //为intent添加额外的信息
                intent.putExtra("useName", etx.getText().toString());
                //启动Activity
                startActivity(intent);
            }
        });
    }
}
