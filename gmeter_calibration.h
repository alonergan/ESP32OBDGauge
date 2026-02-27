#ifndef GMETER_CALIBRATION_H
#define GMETER_CALIBRATION_H

#include <math.h>
#include <Adafruit_Sensor.h>

struct Vec3 {
    float x;
    float y;
    float z;
};

class GMeterCalibration {
public:
    GMeterCalibration() { reset(); }

    void reset() {
        calibrated = false;
        collecting = false;
        sampleCount = 0;
        sum = {0.0f, 0.0f, 0.0f};
        bias = {0.0f, 0.0f, 0.0f};
        setIdentity(rotation);
    }

    void startCalibration() {
        collecting = true;
        calibrated = false;
        sampleCount = 0;
        sum = {0.0f, 0.0f, 0.0f};
    }

    void addSample(const sensors_event_t& accel) {
        if (!collecting) {
            return;
        }

        sum.x += accel.acceleration.x / 9.80665f;
        sum.y += accel.acceleration.y / 9.80665f;
        sum.z += accel.acceleration.z / 9.80665f;
        sampleCount++;

        if (sampleCount >= REQUIRED_SAMPLES) {
            finalizeCalibration();
        }
    }

    bool isCollecting() const { return collecting; }
    bool isCalibrated() const { return calibrated; }
    int getSampleCount() const { return sampleCount; }
    int getRequiredSamples() const { return REQUIRED_SAMPLES; }

    Vec3 normalize(const sensors_event_t& accel) const {
        Vec3 raw = {
            accel.acceleration.x / 9.80665f,
            accel.acceleration.y / 9.80665f,
            accel.acceleration.z / 9.80665f
        };

        Vec3 rotated = multiply(rotation, raw);
        rotated.x -= bias.x;
        rotated.y -= bias.y;
        rotated.z -= bias.z;
        return rotated;
    }

    void getCalibration(float outRotation[3][3], Vec3& outBias) const {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                outRotation[i][j] = rotation[i][j];
            }
        }
        outBias = bias;
    }

    void setCalibration(const float inRotation[3][3], const Vec3& inBias) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                rotation[i][j] = inRotation[i][j];
            }
        }
        bias = inBias;
        calibrated = true;
        collecting = false;
        sampleCount = 0;
    }

private:
    static const int REQUIRED_SAMPLES = 250;
    bool calibrated;
    bool collecting;
    int sampleCount;
    Vec3 sum;
    Vec3 bias;
    float rotation[3][3];

    static void setIdentity(float m[3][3]) {
        m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f;
        m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f;
        m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f;
    }

    static float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    static float norm(const Vec3& v) {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    static Vec3 normalizeVec(const Vec3& v) {
        float n = norm(v);
        if (n < 1e-6f) {
            return {0.0f, 0.0f, 1.0f};
        }
        return {v.x / n, v.y / n, v.z / n};
    }

    static Vec3 multiply(const float m[3][3], const Vec3& v) {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
        };
    }

    void finalizeCalibration() {
        collecting = false;

        Vec3 gravity = {
            sum.x / sampleCount,
            sum.y / sampleCount,
            sum.z / sampleCount
        };

        Vec3 from = normalizeVec(gravity);
        Vec3 to = {0.0f, 0.0f, 1.0f};
        Vec3 axis = cross(from, to);
        float axisNorm = norm(axis);
        float c = dot(from, to);

        if (axisNorm < 1e-6f) {
            setIdentity(rotation);
        } else {
            axis = {axis.x / axisNorm, axis.y / axisNorm, axis.z / axisNorm};
            float s = axisNorm;
            float t = 1.0f - c;

            rotation[0][0] = c + axis.x * axis.x * t;
            rotation[0][1] = axis.x * axis.y * t - axis.z * s;
            rotation[0][2] = axis.x * axis.z * t + axis.y * s;
            rotation[1][0] = axis.y * axis.x * t + axis.z * s;
            rotation[1][1] = c + axis.y * axis.y * t;
            rotation[1][2] = axis.y * axis.z * t - axis.x * s;
            rotation[2][0] = axis.z * axis.x * t - axis.y * s;
            rotation[2][1] = axis.z * axis.y * t + axis.x * s;
            rotation[2][2] = c + axis.z * axis.z * t;
        }

        Vec3 baseline = multiply(rotation, gravity);
        bias.x = baseline.x;
        bias.y = baseline.y;
        bias.z = baseline.z - 1.0f;
        calibrated = true;
    }
};

#endif
