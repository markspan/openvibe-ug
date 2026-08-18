///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDefinesShimmerGSR.hpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------


#pragma once

#if defined TARGET_OS_Windows

namespace OpenViBE {
namespace AcquisitionServer {

#define DATA_PACKET 0x00
#define INQUIRY_COMMAND 0x01
#define INQUIRY_RESPONSE 0x02
#define GET_SAMPLING_RATE_COMMAND 0x03
#define SAMPLING_RATE_RESPONSE 0x04
#define SET_SAMPLING_RATE_COMMAND 0x05
#define TOGGLE_LED_COMMAND 0x06
#define START_STREAMING_COMMAND 0x07
#define SET_SENSORS_COMMAND 0x08
#define SET_ACCEL_RANGE_COMMAND 0x09
#define ACCEL_RANGE_RESPONSE 0x0A
#define GET_ACCEL_RANGE_COMMAND 0x0B
#define SET_5V_REGULATOR_COMMAND 0x0C
#define SET_POWER_MUX_COMMAND 0x0D
#define SET_CONFIG_SETUP_BYTE0_Command 0x0E
#define CONFIG_SETUP_BYTE0_RESPONSE 0x0F

#define GET_CONFIG_SETUP_BYTE0_COMMAND 0x10
#define SET_ACCEL_CALIBRATION_COMMAND 0x11
#define ACCEL_CALIBRATION_RESPONSE 0x12
#define GET_ACCEL_CALIBRATION_COMMAND 0x13
#define SET_GYRO_CALIBRATION_COMMAND 0x14
#define GYRO_CALIBRATION_RESPONSE 0x15
#define GET_GYRO_CALIBRATION_COMMAND 0x16
#define SET_MAG_CALIBRATION_COMMAND 0x17
#define MAG_CALIBRATION_RESPONSE 0x18
#define GET_MAG_CALIBRATION_COMMAND 0x19
#define WR_ACCEL_CALIBRATION_RESPONSE 0x1B

#define STOP_STREAMING_COMMAND 0x20
#define SET_GSR_RANGE_COMMAND 0x21
#define GSR_RANGE_RESPONSE 0x22
#define GET_GSR_RANGE_COMMAND 0x23

#define GET_SHIMMER_VERSION_RESPONSE 0x25
#define SET_EMG_CALIBRATION_COMMAND 0x26
#define EMG_CALIBRATION_RESPONSE 0x27
#define GET_EMG_CALIBRATION_COMMAND 0x28
#define SET_ECG_CALIBRATION_COMMAND 0x29
#define ECG_CALIBRATION_RESPONSE 0x2A
#define GET_ECG_CALIBRATION_COMMAND 0x2B
#define GET_ALL_CALIBRATION_COMMAND 0x2C
#define ALL_CALIBRATION_RESPONSE 0x2D
#define GET_FW_VERSION_COMMAND 0x2E
#define FW_VERSION_RESPONSE 0x2F

#define SET_BLINK_LED 0x30
#define BLINK_LED_RESPONSE 0x31
#define GET_BLINK_LED 0x32
#define SET_GYRO_TEMP_VREF_COMMAND 0x33
#define SET_BUFFER_SIZE_COMMAND 0x34
#define BUFFER_SIZE_RESPONSE 0x35
#define GET_BUFFER_SIZE_COMMAND 0x36
#define SET_MAG_GAIN_COMMAND 0x37
#define MAG_GAIN_RESPONSE 0x38
#define GET_MAG_GAIN_COMMAND 0x39
#define SET_MAG_SAMPLING_RATE_COMMAND 0x3A
#define MAG_SAMPLING_RATE_RESPONSE 0x3B
#define GET_MAG_SAMPLING_RATE_COMMAND 0x3C
#define GET_SHIMMER_VERSION_COMMAND 0x3F

#define SET_ACCEL_SAMPLING_RATE_COMMAND 0x40
#define ACCEL_SAMPLING_RATE_RESPONSE 0x41
#define GET_ACCEL_SAMPLING_RATE_COMMAND 0x42
#define MPU9150_GYRO_RANGE_RESPONSE 0x4A
#define GET_MPU9150_GYRO_RANGE_COMMAND 0x4B
#define SET_MPU9150_SAMPLING_RATE_COMMAND 0x4C
#define SET_MPU9150_GYRO_RANGE_COMMAND 0x49

#define SET_BMP180_PRES_RESOLUTION_COMMAND 0x52
#define BMP180_PRES_RESOLUTION_RESPONSE 0x53
#define GET_BMP180_PRES_RESOLUTION_COMMAND 0x54
#define BMP180_CALIBRATION_COEFFICIENTS_RESPONSE 0x58
#define GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND 0x59
#define SET_INTERNAL_EXP_POWER_ENABLE_COMMAND 0x5E
#define INTERNAL_EXP_POWER_ENABLE_RESPONSE 0x5F

#define GET_INTERNAL_EXP_POWER_ENABLE_COMMAND 0x60
#define SET_EXG_REGS_COMMAND 0x61
#define EXG_REGS_RESPONSE 0x62
#define GET_EXG_REGS_COMMAND 0x63
#define SET_BAUD_RATE_COMMAND 0x6A
#define BAUD_RATE_RESPONSE 0X6B
#define GET_BAUD_RATE_COMMAND 0X6C
#define DETECT_EXPANSION_BOARD_RESPONSE 0X65
#define GET_EXPANSION_BOARD_COMMAND 0x66

#define START_SDBT_COMMAND 0x70
#define STATUS_RESPONSE 0x71
#define GET_STATUS_COMMAND 0x72
#define SET_TRIAL_CONFIG_COMMAND 0x73
#define TRIAL_CONFIG_RESPONSE 0x74
#define GET_TRIAL_CONFIG_COMMAND 0x75
#define SET_CENTER_COMMAND 0x76
#define CENTER_RESPONSE 0x77
#define GET_CENTER_COMMAND 0x78
#define SET_SHIMMERNAME_COMMAND 0x79
#define SHIMMERNAME_RESPONSE 0x7a
#define GET_SHIMMERNAME_COMMAND 0x7b
#define SET_EXPID_COMMAND 0x7c
#define EXPID_RESPONSE 0x7d
#define GET_EXPID_COMMAND 0x7e
#define SET_MYID_COMMAND 0x7F

#define MYID_RESPONSE 0x80
#define GET_MYID_COMMAND 0x81
#define SET_NSHIMMER_COMMAND 0x82
#define NSHIMMER_RESPONSE 0x83
#define GET_NSHIMMER_COMMAND 0x84
#define SET_CONFIGTIME_COMMAND 0x85
#define CONFIGTIME_RESPONSE 0x86
#define GET_CONFIGTIME_COMMAND 0x87
#define DIR_RESPONSE 0x88
#define GET_DIR_COMMAND 0x89
#define INSTREAM_CMD_RESPONSE 0x8A
#define SET_RWC_COMMAND 0x8F

#define RWC_RESPONSE 0x90
#define GET_RWC_COMMAND 0x91
#define VBATT_RESPONSE 0x94
#define GET_VBATT_COMMAND 0x95
#define SET_VBATT_FREQ_COMMAND 0x98
#define VBATT_FREQ_RESPONSE 0x99
#define GET_VBATT_FREQ_COMMAND 0x9A

#define GetBmp280CalibrationCoefficientsCommand 159

#define ACK_PROCESSED 0xFF

#define MAX_NUMBER_OF_SIGNALS 35

#define LOW_NOISE_ACCELEROMETER_X "Low Noise Accelerometer X"
#define LOW_NOISE_ACCELEROMETER_Y "Low Noise Accelerometer Y"
#define LOW_NOISE_ACCELEROMETER_Z "Low Noise Accelerometer Z"
#define V_SENSE_BATT "VSenseBatt"
#define WIDE_RANGE_ACCELEROMETER_X "Wide Range Accelerometer X"
#define WIDE_RANGE_ACCELEROMETER_Y "Wide Range Accelerometer Y"
#define WIDE_RANGE_ACCELEROMETER_Z "Wide Range Accelerometer Z"
#define MAGNETOMETER_X "Magnetometer X"
#define MAGNETOMETER_Y "Magnetometer Y"
#define MAGNETOMETER_Z "Magnetometer Z"
#define GYROSCOPE_X "Gyroscope X"
#define GYROSCOPE_Y "Gyroscope Y"
#define GYROSCOPE_Z "Gyroscope Z"
#define EXTERNAL_ADC_A7 "External ADC A7"
#define EXTERNAL_ADC_A6 "External ADC A6"
#define EXTERNAL_ADC_A15 "External ADC A15"
#define INTERNAL_ADC_A1 "Internal ADC A1"
#define INTERNAL_ADC_A12 "Internal ADC A12"
#define INTERNAL_ADC_A13 "Internal ADC A13"
#define INTERNAL_ADC_A14 "Internal ADC A14"
#define PRESSURE "Pressure"
#define TEMPERATURE "Temperature"
#define GSR "GSR"
#define GSR_CONDUCTANCE "GSR Conductance"
#define EXG1_STATUS "EXG1 Status"
#define EXG2_STATUS "EXG2 Status"
#define ECG_LL_RA "ECG LL-RA"
#define ECG_LA_RA "ECG LA-RA"
#define ECG_VX_RL "ECG Vx-RL"
#define EMG_CH1 "EMG CH1"
#define EMG_CH2 "EMG CH2"
#define EXG1_CH1 "EXG1 CH1"
#define EXG1_CH2 "EXG1 CH2"
#define EXG2_CH1 "EXG2 CH1"
#define EXG2_CH2 "EXG2 CH2"
#define EXG1_CH1_16BIT "EXG1 CH1 16Bit"
#define EXG1_CH2_16BIT "EXG1 CH2 16Bit"
#define EXG2_CH1_16BIT "EXG2 CH1 16Bit"
#define EXG2_CH2_16BIT "EXG2 CH2 16Bit"
#define BRIGE_AMPLIFIER_HIGH "Bridge Amplifier High"
#define BRIGE_AMPLIFIER_LOW "Bridge Amplifier Low"
#define QUATERNION_0 "Quaternion 0"
#define QUATERNION_1 "Quaternion 1"
#define QUATERNION_2 "Quaternion 2"
#define QUATERNION_3 "Quaternion 3"
#define AXIS_ANGLE_A "Axis Angle A"
#define AXIS_ANGLE_X "Axis Angle X"
#define AXIS_ANGLE_Y "Axis Angle Y"
#define AXIS_ANGLE_Z "Axis Angle Z"

#define SENSOR_A_ACCEL 0x80
#define SENSOR_MPU9150_GYRO 0x040
#define SENSOR_LSM303DLHC_MAG 0x20
#define SENSOR_GSR 0x04
#define SENSOR_EXT_A7 0x02
#define SENSOR_EXT_A6 0x01
#define SENSOR_VBATT 0x2000
#define SENSOR_D_ACCEL 0x1000
#define SENSOR_EXT_A15 0x0800
#define SENSOR_INT_A1 0x0400
#define SENSOR_INT_A12 0x0200
#define SENSOR_INT_A13 0x0100
#define SENSOR_INT_A14 0x800000
#define SENSOR_BMP180_PRESSURE 0x40000
#define SENSOR_EXG1_24BIT 0x10
#define SENSOR_EXG2_24BIT 0x08
#define SENSOR_EXG1_16BIT 0x100000
#define SENSOR_EXG2_16BIT 0x080000
#define SENSOR_BRIDGE_AMP 0x8000

}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif  // TARGET_OS_Windows
