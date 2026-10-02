package com.example.precisionclock
object NativeBridge {
    init { System.loadLibrary("clock") }
    external fun start()
    external fun stop()
    external fun requestSync()
    external fun nowServerNs(): Long
    external fun offsetNs(): Long
    external fun rttNs(): Long
    external fun jitterNs(): Long
    external fun sampleCount(): Int
    external fun syncStatus(): Int
    external fun lastSyncAgeMs(): Long
}
