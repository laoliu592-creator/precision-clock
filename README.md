# 时序 — V1.0 — Android 10+, arm64-v8a

This is the integrated V1.0 engineering build for the precision countdown clock.

## Included

- Android 10+ / API 29+
- arm64-v8a only
- Kotlin UI + C++20 NDK timing core
- `CLOCK_MONOTONIC` extrapolation between syncs
- UDP NTP with `recvmsg()`
- `SO_TIMESTAMPNS` with `SO_TIMESTAMP` fallback
- T1/T2/T3/T4 offset and delay calculation
- NTP response validation: version, mode, leap indicator, stratum, originate timestamp and non-zero server timestamps
- DNS-based IPv4 NTP server resolution
- Five-server sampling and filtering
- RTT and jitter metrics
- Network-change-triggered resync
- Foreground service with Android special-use FGS declaration
- Overlay clock and countdown
- Target time persistence
- 50 ms UI/overlay refresh (display only; native clock is monotonic)
- GitHub Actions debug artifact

## Sync status

`0` unsynced, `1` syncing, `2` synced, `3` degraded.

A failed refresh does not throw away the last valid calibration. The clock continues from the last monotonic anchor and the status becomes degraded until a new sample set succeeds.

## Important release note

The GitHub release workflow currently produces an **unsigned** release APK. A distributable production APK still needs your own Android signing key and GitHub Actions secrets. Do not commit the keystore or passwords.

## NTP servers

The native worker queries:

- time.cloudflare.com
- time.google.com
- pool.ntp.org
- time.windows.com
- time.apple.com

The app uses the lowest-delay sample that is consistent with the median-offset sample set.


## UI
The V1.0 home screen uses a compact dark precision-clock layout: large synchronized time, prominent countdown, NTP health card, target editor, and two primary actions.
