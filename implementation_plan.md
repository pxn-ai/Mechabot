# MPU6500 Sensor Fusion + Standalone HTML UI

## Background

The **MPU6500** is a 6-axis IMU (3-axis gyro + 3-axis accel). It does NOT have a magnetometer, so it cannot compute absolute heading alone. However, when fused with the existing **HMC5983 compass**:

| Sensor | Strength | Weakness |
|--------|----------|----------|
| **HMC5983 Compass** | Absolute heading (no drift) | Slow (~75Hz), noisy, susceptible to nearby motors |
| **MPU6500 Gyro** | Ultra-fast, smooth yaw rate | Drifts over time (accumulated integration error) |
| **Complementary Filter** | Best of both worlds | — |

**Fusion formula**: `fusedYaw = α × (fusedYaw + gyroZ × dt) + (1 − α) × compassHeading`  
Where `α ≈ 0.95` trusts the gyro for short-term changes and the compass corrects long-term drift.

## Proposed Changes

### IMU Module
#### [NEW] [imu.h](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/include/imu.h)
- MPU6500 via `MPU9250_WE` library (supports MPU6500 natively)
- Same I2C bus as compass (SDA=GPIO1, SCL=GPIO2, MPU6500 address 0x68)
- Auto-calibrate gyro/accel at startup (keep robot still for 2 seconds)
- Expose: `getGyroZ()` (°/s), `getPitch()`, `getRoll()` from accel

---

### Sensor Fusion Upgrade
#### [MODIFY] [compass.h](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/include/compass.h)
- Rename to a more general "navigation" approach or keep compass but add fusion
- `getFusedHeading()` — complementary filter combining gyro yaw rate + compass absolute heading
- `getHeading()` stays as raw compass-only (for calibration)
- The fused heading becomes the primary source for PA-Lock and precise turns

---

### Firmware Updates
#### [MODIFY] [main.cpp](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/src/main.cpp)
- Initialize MPU6500 in `setup()`, run gyro calibration
- Replace `getHeading()` usage in the P-controller and precise turn with `getFusedHeading()`
- Add `/imu` REST endpoint returning pitch, roll, gyroZ, fusedHeading, raw compass heading
- Add `/imu_calibrate` endpoint to re-run gyro zeroing

---

### Standalone HTML (Browser-Testable UI)
#### [NEW] [ui_preview.html](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/ui_preview.html)
- Exact copy of the embedded UI HTML
- Uses **mock data** (simulated heading, fake `/heading` and `/imu` responses) so it works standalone in Chrome
- **Workflow**: Edit `ui_preview.html` → preview in Chrome → once finalized, sync content into `web_ui.h`

#### [MODIFY] [web_ui.h](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/include/web_ui.h)
- Add IMU telemetry display in center HUD: pitch/roll gauges, gyro rate indicator
- Add gyro calibration button
- All changes authored in `ui_preview.html` first, then copied to `web_ui.h`

---

### Dependencies
#### [MODIFY] [platformio.ini](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/platformio.ini)
```
wollewald/MPU9250_WE@^1.2.17
```

## User Review Required

> [!IMPORTANT]
> **I2C Bus Sharing**: Both HMC5983 (0x1E) and MPU6500 (0x68) will share the same I2C bus on GPIO1/GPIO2. The addresses don't conflict, but confirm both sensors are physically wired to the same SDA/SCL lines.

> [!IMPORTANT]
> **MPU6500 vs MPU6050**: The `MPU9250_WE` library supports both. If your chip is actually an MPU6050 (common clone), it still works identically. Confirm which chip you have.

## Verification Plan

### Automated Tests
- `pio run` — compile check

### Manual Verification
- Flash, open serial monitor → should see `[IMU] MPU6500 ready` and `[COMPASS] HMC5983 ready`
- Check `/imu` endpoint returns valid pitch/roll/yaw data
- Compare raw compass heading vs fused heading in the UI — fused should be smoother
- Execute a precise 90° turn — should be significantly faster and more accurate with gyro-assisted PID
