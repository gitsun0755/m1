package com.sm.framebyframe;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
//import android.app.Activity;

import android.graphics.drawable.AnimationDrawable;

//import android.os.Bundle;

import android.view.View;

import android.view.View.OnClickListener;

import android.widget.Button;

import android.widget.ImageView;
public class MainActivity extends AppCompatActivity {
    private Button button2 = null;

    private ImageView imageView = null;

    @Override

    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        button2 = (Button)findViewById(R.id.buttonx);

        imageView = (ImageView)findViewById(R.id.image);

        button2.setOnClickListener(new ButtonListener());

    }

    class ButtonListener implements OnClickListener{

        public void onClick(View v) {

            imageView.setBackgroundResource(R.anim.anim);

            AnimationDrawable animationDrawable = (AnimationDrawable) imageView.getBackground();

            animationDrawable.start();

        }

    }

}