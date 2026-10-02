package com.example.precisionclock

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import java.time.Instant
import java.time.LocalDateTime
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.time.format.DateTimeParseException
import java.util.Locale

class MainActivity : AppCompatActivity() {
    companion object { private const val PREFS = "clock"; private const val TARGET = "target_epoch_ns" }
    private val handler = Handler(Looper.getMainLooper())
    private lateinit var clockText: TextView
    private lateinit var statusText: TextView
    private lateinit var metricsText: TextView
    private lateinit var targetText: TextView
    private lateinit var targetInput: EditText
    private lateinit var targetLabel: TextView
    private lateinit var lastSyncText: TextView
    private lateinit var headerStatus: TextView
    private lateinit var overlayButton: Button
    private val displayFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.SSS", Locale.US)
    private var targetEpochNs = 0L
    private var overlayOn = false
    private val ticker = object : Runnable { override fun run() { render(); handler.postDelayed(this, 50L) } }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        clockText = findViewById(R.id.clock); statusText = findViewById(R.id.status); metricsText = findViewById(R.id.metrics); targetText = findViewById(R.id.target); targetInput = findViewById(R.id.target_input); targetLabel = findViewById(R.id.target_label); lastSyncText = findViewById(R.id.last_sync); headerStatus = findViewById(R.id.header_status); overlayButton = findViewById(R.id.overlay_button)
        targetEpochNs = getSharedPreferences(PREFS, MODE_PRIVATE).getLong(TARGET, 0L)
        if (targetEpochNs > 0L) targetInput.setText(formatLocal(targetEpochNs))
        if (android.os.Build.VERSION.SDK_INT >= 33 && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 100)
        findViewById<Button>(R.id.sync_button).setOnClickListener { NativeBridge.requestSync(); Toast.makeText(this, "已请求立即校时", Toast.LENGTH_SHORT).show() }
        findViewById<Button>(R.id.target_button).setOnClickListener { setTargetFromInput() }
        findViewById<Button>(R.id.next_minute_button).setOnClickListener { val now = System.currentTimeMillis(); val next = ((now / 60_000L) + 1L) * 60_000L; saveTarget(next * 1_000_000L) }
        overlayButton.setOnClickListener { toggleOverlay() }
        ContextCompat.startForegroundService(this, Intent(this, ClockService::class.java))
    }
    override fun onResume() { super.onResume(); updateOverlayButton(); handler.removeCallbacks(ticker); handler.post(ticker) }
    override fun onPause() { handler.removeCallbacks(ticker); super.onPause() }
    private fun render() {
        val nowNs = NativeBridge.nowServerNs().takeIf { it > 0L } ?: System.currentTimeMillis() * 1_000_000L
        clockText.text = formatLocal(nowNs)
        val status = NativeBridge.syncStatus()
        val statusLabel = when (status) {
            2 -> "● NTP 已同步"
            1 -> "● 正在校时…"
            3 -> "● 沿用最近校准"
            else -> "○ 尚未同步"
        }
        statusText.text = statusLabel
        headerStatus.text = when (status) { 2 -> "已同步"; 1 -> "校时中"; 3 -> "降级"; else -> "未同步" }
        val age = NativeBridge.lastSyncAgeMs()
        lastSyncText.text = "最近校时 ${formatAge(age)}"
        metricsText.text = "Offset  ${formatMs(NativeBridge.offsetNs())}\nRTT     ${formatMs(NativeBridge.rttNs())}\nJitter  ${formatMs(NativeBridge.jitterNs())}\nSamples ${NativeBridge.sampleCount()}"
        if (targetEpochNs > 0L) {
            val remain = targetEpochNs - nowNs
            targetText.text = formatDuration(remain)
            targetLabel.text = "目标  ${formatLocal(targetEpochNs)}"
        } else {
            targetText.text = "--:--:--.---"
            targetLabel.text = "尚未设置目标"
        }
    }
    private fun setTargetFromInput() {
        val text = targetInput.text.toString().trim()
        val patterns = listOf("yyyy-MM-dd HH:mm:ss.SSS", "yyyy-MM-dd HH:mm:ss")
        var epoch: Long? = null
        for (pattern in patterns) try { epoch = LocalDateTime.parse(text, DateTimeFormatter.ofPattern(pattern)).atZone(ZoneId.systemDefault()).toInstant().toEpochMilli() * 1_000_000L; break } catch (_: DateTimeParseException) {}
        if (epoch == null) { Toast.makeText(this, "格式：2026-10-02 18:00:00.000", Toast.LENGTH_LONG).show(); return }
        saveTarget(epoch)
    }
    private fun saveTarget(epochNs: Long) { targetEpochNs = epochNs; getSharedPreferences(PREFS, MODE_PRIVATE).edit().putLong(TARGET, epochNs).apply(); targetInput.setText(formatLocal(epochNs)); render() }
    private fun formatLocal(ns: Long): String = displayFormatter.withZone(ZoneId.systemDefault()).format(Instant.ofEpochSecond(ns / 1_000_000_000L, ns % 1_000_000_000L))
    private fun formatMs(ns: Long): String = String.format(Locale.US, "%+.3f ms", ns / 1_000_000.0)
    private fun formatAge(ms: Long): String = if (ms < 0) "--" else if (ms < 1000) "刚刚" else if (ms < 60_000) "${ms / 1000}s 前" else "${ms / 60_000}m 前"
    private fun formatDuration(ns: Long): String { val sign = if (ns < 0) "-" else ""; val ms = kotlin.math.abs(ns) / 1_000_000L; val h = ms / 3_600_000L; val m = (ms / 60_000L) % 60L; val s = (ms / 1_000L) % 60L; return String.format(Locale.US, "%s%02d:%02d:%02d.%03d", sign, h, m, s, ms % 1_000L) }
    private fun toggleOverlay() {
        if (!Settings.canDrawOverlays(this)) { startActivity(Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:$packageName"))); return }
        overlayOn = !overlayOn
        startService(Intent(this, ClockService::class.java).putExtra(ClockService.EXTRA_OVERLAY, overlayOn))
        updateOverlayButton()
    }
    private fun updateOverlayButton() { overlayButton.text = if (Settings.canDrawOverlays(this)) if (overlayOn) "关闭悬浮时钟" else "开启悬浮时钟" else "授予悬浮窗权限" }
}
