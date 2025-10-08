#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

/**
 * Telemetry data structure emitted by satellite.
 * Contains sensor readings and attitude information.
 */
struct Telemetry {
    std::chrono::steady_clock::time_point ts;
    double temperature_c;      // Temperature in Celsius
    double battery_pct;        // Battery level percentage
    double orbit_altitude_km;  // Orbital altitude in kilometers
    double pitch_deg;          // Pitch angle in degrees
    double yaw_deg;            // Yaw angle in degrees
    double roll_deg;           // Roll angle in degrees

    /**
     * Serialize to simple key=value format (lighter than JSON).
     * Format: "ts=<nanos>|temp=<val>|batt=<val>|alt=<val>|pitch=<val>|yaw=<val>|roll=<val>"
     */
    std::string to_json() const {
        std::ostringstream oss;
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            ts.time_since_epoch()).count();

        oss << std::fixed << std::setprecision(2);
        oss << "ts=" << nanos
            << "|temp=" << temperature_c
            << "|batt=" << battery_pct
            << "|alt=" << orbit_altitude_km
            << "|pitch=" << pitch_deg
            << "|yaw=" << yaw_deg
            << "|roll=" << roll_deg;

        return oss.str();
    }

    /**
     * Deserialize from key=value format.
     */
    static Telemetry from_json(const std::string& s) {
        Telemetry t;
        std::istringstream iss(s);
        std::string token;
        long long ts_nanos = 0;

        while (std::getline(iss, token, '|')) {
            auto eq_pos = token.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string key = token.substr(0, eq_pos);
            std::string val = token.substr(eq_pos + 1);

            if (key == "ts") {
                ts_nanos = std::stoll(val);
            } else if (key == "temp") {
                t.temperature_c = std::stod(val);
            } else if (key == "batt") {
                t.battery_pct = std::stod(val);
            } else if (key == "alt") {
                t.orbit_altitude_km = std::stod(val);
            } else if (key == "pitch") {
                t.pitch_deg = std::stod(val);
            } else if (key == "yaw") {
                t.yaw_deg = std::stod(val);
            } else if (key == "roll") {
                t.roll_deg = std::stod(val);
            }
        }

        t.ts = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(ts_nanos));
        return t;
    }

    /**
     * Format as CSV line for logging.
     */
    std::string to_csv() const {
        std::ostringstream oss;
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            ts.time_since_epoch()).count();

        oss << std::fixed << std::setprecision(2);
        oss << nanos << ","
            << temperature_c << ","
            << battery_pct << ","
            << orbit_altitude_km << ","
            << pitch_deg << ","
            << yaw_deg << ","
            << roll_deg;

        return oss.str();
    }

    /**
     * Get CSV header.
     */
    static std::string csv_header() {
        return "timestamp_ns,temperature_c,battery_pct,orbit_altitude_km,pitch_deg,yaw_deg,roll_deg";
    }
};
