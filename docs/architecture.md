# Architecture

UI and Android lifecycle stay in Kotlin. Time measurement, NTP packet handling,
POSIX UDP, kernel receive timestamps, filtering and monotonic clock extrapolation
belong to native C++.

Core path:

NTP -> UDP socket -> recvmsg -> kernel timestamp -> NTP sample -> ClockState
-> CLOCK_MONOTONIC extrapolation -> JNI snapshot -> UI
