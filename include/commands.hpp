#pragma once

#include <string>
#include <sstream>
#include <stdexcept>

/**
 * Command types that ground station can send to satellite.
 */
enum class CommandType {
    AdjustOrientation,  // Adjust pitch/yaw/roll
    ThrustBurn,         // Perform a thrust burn for specified duration
    EnterSafeMode,      // Force satellite into safe mode
    Reboot              // Reboot satellite systems
};

/**
 * Command structure with type and optional parameters.
 */
struct Command {
    CommandType type;

    // Orientation adjustment deltas (degrees)
    double d_pitch = 0.0;
    double d_yaw = 0.0;
    double d_roll = 0.0;

    // Thrust burn duration (seconds)
    double burn_seconds = 0.0;

    /**
     * Serialize command to string format: "TYPE|param1|param2|..."
     */
    std::string serialize() const {
        std::ostringstream oss;
        switch (type) {
            case CommandType::AdjustOrientation:
                oss << "ADJUST_ORIENTATION|" << d_pitch << "|" << d_yaw << "|" << d_roll;
                break;
            case CommandType::ThrustBurn:
                oss << "THRUST_BURN|" << burn_seconds;
                break;
            case CommandType::EnterSafeMode:
                oss << "ENTER_SAFE_MODE";
                break;
            case CommandType::Reboot:
                oss << "REBOOT";
                break;
        }
        return oss.str();
    }

    /**
     * Deserialize command from string format.
     */
    static Command deserialize(const std::string& s) {
        Command cmd;
        std::istringstream iss(s);
        std::string type_str;

        if (!std::getline(iss, type_str, '|')) {
            throw std::runtime_error("Invalid command format");
        }

        if (type_str == "ADJUST_ORIENTATION") {
            cmd.type = CommandType::AdjustOrientation;
            char delim;
            if (!(iss >> cmd.d_pitch >> delim >> cmd.d_yaw >> delim >> cmd.d_roll)) {
                throw std::runtime_error("Invalid AdjustOrientation parameters");
            }
        } else if (type_str == "THRUST_BURN") {
            cmd.type = CommandType::ThrustBurn;
            if (!(iss >> cmd.burn_seconds)) {
                throw std::runtime_error("Invalid ThrustBurn parameters");
            }
        } else if (type_str == "ENTER_SAFE_MODE") {
            cmd.type = CommandType::EnterSafeMode;
        } else if (type_str == "REBOOT") {
            cmd.type = CommandType::Reboot;
        } else {
            throw std::runtime_error("Unknown command type: " + type_str);
        }

        return cmd;
    }

    /**
     * Get human-readable command name.
     */
    std::string name() const {
        switch (type) {
            case CommandType::AdjustOrientation: return "AdjustOrientation";
            case CommandType::ThrustBurn: return "ThrustBurn";
            case CommandType::EnterSafeMode: return "EnterSafeMode";
            case CommandType::Reboot: return "Reboot";
            default: return "Unknown";
        }
    }
};
