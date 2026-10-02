package com.example.precisionclock

import android.app.*
import android.content.*
import android.content.pm.ServiceInfo
import android.graphics.Color
import android.graphics.PixelFormat
import android.net.ConnectivityManager
import android.net.Network
import android.os.*
import android.provider.Settings
import android.view.*
import android.widget.TextView
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.util.Locale

class ClockService : Service() {
    companion object { const val EXTRA_OVERLAY = "overlay"; private const val CHANNEL = "clock"; private const val ID = 1001 }
    private lateinit var wm: WindowManager
    private lateinit var connectivity: ConnectivityManager
    private var overlay: TextView? = null
    private val handler = Handler(Looper.getMainLooper())
    private val prefs by lazy { getSharedPreferences("clock", MODE_PRIVATE) }
    private val fmt = DateTimeFormatter.ofPattern("HH:mm:ss.SSS", Locale.US).withZone(ZoneId.systemDefault())
    private val updater = object : Runnable { override fun run() { updateOverlay(); handler.postDelayed(this, 50L) } }
    private val networkCallback = object : ConnectivityManager.NetworkCallback() { override fun onAvailable(network: Network) { NativeBridge.requestSync() }; override fun onCapabilitiesChanged(network: Network, caps: android.net.NetworkCapabilities) { NativeBridge.requestSync() } }
    override fun onCreate() {
        super.onCreate(); createChannel(); wm = getSystemService(WINDOW_SERVICE) as WindowManager; connectivity = getSystemService(ConnectivityManager::class.java)
        if (Build.VERSION.SDK_INT >= 34) startForeground(ID, notification(), ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE) else startForeground(ID, notification())
        NativeBridge.start()
        runCatching { connectivity.registerDefaultNetworkCallback(networkCallback) }
    }
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int { intent?.getBooleanExtra(EXTRA_OVERLAY, false)?.let { if (it) showOverlay() else hideOverlay() }; return START_STICKY }
    override fun onDestroy() { hideOverlay(); runCatching { connectivity.unregisterNetworkCallback(networkCallback) }; NativeBridge.stop(); super.onDestroy() }
    override fun onBind(intent: Intent?): IBinder? = null
    private fun showOverlay() {
        if (!Settings.canDrawOverlays(this) || overlay != null) return
        prefs.edit().putBoolean("overlay_enabled", true).apply()
        val tv = TextView(this).apply { setTextColor(Color.WHITE); setBackgroundColor(0xDD101418.toInt()); textSize = 22f; setPadding(18, 10, 18, 10); text = "--:--:--.---\n未同步" }
        val type = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
        val p = WindowManager.LayoutParams(WindowManager.LayoutParams.WRAP_CONTENT, WindowManager.LayoutParams.WRAP_CONTENT, type, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, PixelFormat.TRANSLUCENT).apply { gravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL; y = 72 }
        runCatching { wm.addView(tv, p); overlay = tv; handler.removeCallbacks(updater); handler.post(updater) }
    }
    private fun hideOverlay() { handler.removeCallbacks(updater); overlay?.let { runCatching { wm.removeView(it) } }; overlay = null; prefs.edit().putBoolean("overlay_enabled", false).apply() }
    private fun updateOverlay() {
        val view = overlay ?: return; val now = NativeBridge.nowServerNs(); val target = prefs.getLong("target_epoch_ns", 0L)
        if (now <= 0L) { view.text = "--:--:--.---\nNTP 同步中"; return }
        val countdown = if (target > 0L) "\n${formatDuration(target - now)}" else ""
        view.text = fmt.format(Instant.ofEpochSecond(now / 1_000_000_000L, now % 1_000_000_000L)) + countdown
    }
    private fun formatDuration(ns: Long): String { val sign = if (ns < 0) "-" else ""; val ms = kotlin.math.abs(ns) / 1_000_000L; return String.format(Locale.US, "%s%02d:%02d:%02d.%03d", sign, ms / 3_600_000L, (ms / 60_000L) % 60L, (ms / 1_000L) % 60L, ms % 1_000L) }
    private fun createChannel() { getSystemService(NotificationManager::class.java).createNotificationChannel(NotificationChannel(CHANNEL, "Precision Clock", NotificationManager.IMPORTANCE_LOW)) }
    private fun notification(): Notification = Notification.Builder(this, CHANNEL).setContentTitle("Precision Clock").setContentText("NTP 精准时钟正在运行").setSmallIcon(android.R.drawable.ic_menu_recent_history).setOngoing(true).build()
}
