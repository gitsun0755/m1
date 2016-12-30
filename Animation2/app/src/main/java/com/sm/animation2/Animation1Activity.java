package com.sm.animation2;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.app.Activity;

import android.os.Bundle;

import android.view.View;

import android.view.View.OnClickListener;

import android.view.animation.Animation;

import android.view.animation.AnimationUtils;

import android.widget.Button;

import android.widget.ImageView;


public class Animation1Activity extends Activity {
    private Button rotateButton = null;

    private Button scaleButton = null;

    private Button alphaButton = null;

    private Button translateButton = null;
    private Button doubleButton = null;
    private Button doubleButton2 =null;

    private ImageView image = null;



    @Override

    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_animation1);



        rotateButton = (Button) findViewById(R.id.rotateButton);

        scaleButton = (Button) findViewById(R.id.scaleButton);

        alphaButton = (Button) findViewById(R.id.alphaButton);

        translateButton = (Button) findViewById(R.id.translateButton);

        doubleButton =(Button)findViewById(R.id.DoubleButton);
        doubleButton2 =(Button)findViewById(R.id.MoveButton);
        image = (ImageView) findViewById(R.id.image);



        rotateButton.setOnClickListener(new RotateButtonListener());

        scaleButton.setOnClickListener(new ScaleButtonListener());

        alphaButton.setOnClickListener(new AlphaButtonListener());
        doubleButton.setOnClickListener(new DoubleButtonListener());
        doubleButton2.setOnClickListener(new DoubleButtonListener());
        translateButton.setOnClickListener(new TranslateButtonListener());

    }



    class AlphaButtonListener implements OnClickListener {

        public void onClick(View v) {

            // 使用AnimationUtils装载动画配置文件

            Animation animation = AnimationUtils.loadAnimation(

                    Animation1Activity.this, R.anim.alpha);

            // 启动动画

            image.startAnimation(animation);

        }

    }

    class DoubleButtonListener implements OnClickListener {
        public void onClick(View v) {

            // 使用AnimationUtils装载动画配置文件

            Animation animation = AnimationUtils.loadAnimation(

                    Animation1Activity.this, R.anim.doubleani);

            // 启动动画

            image.startAnimation(animation);

        }

    }

    class DoubleButtonListener2 implements OnClickListener {
        public void onClick(View v) {

            // 使用AnimationUtils装载动画配置文件

            Animation animation = AnimationUtils.loadAnimation(

                    Animation1Activity.this, R.anim.doubleani);

            // 启动动画

            image.startAnimation(animation);

        }

    }

    class RotateButtonListener implements OnClickListener {

        public void onClick(View v) {

            Animation animation = AnimationUtils.loadAnimation(

                    Animation1Activity.this, R.anim.rotate);

            image.startAnimation(animation);

        }

    }



    class ScaleButtonListener implements OnClickListener {

        public void onClick(View v) {

            Animation animation = AnimationUtils.loadAnimation(

                    Animation1Activity.this, R.anim.scale);

            image.startAnimation(animation);

        }

    }



    class TranslateButtonListener implements OnClickListener {

        public void onClick(View v) {

            Animation animation = AnimationUtils.loadAnimation(Animation1Activity.this, R.anim.traslate);

            image.startAnimation(animation);

        }

    }

}