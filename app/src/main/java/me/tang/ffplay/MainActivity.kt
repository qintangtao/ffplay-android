package me.tang.ffplay

import android.media.AudioTrack
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity


class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        var mAudioTrack: AudioTrack

        Thread.currentThread().setName("");
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
    }
}