package com.example.sunming.innovationcourse;

import android.support.v7.app.ActionBarActivity;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class MainActivity extends ActionBarActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TeacherAdapter teacherAdapter = new  TeacherAdapter(this, R.layout.teacher_item, Teacher.getAllTeachers());

        ListView listView = (ListView) findViewById(R.id.teacher_listView);

        listView.setAdapter(teacherAdapter);
    }
}
