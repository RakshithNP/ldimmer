#ifndef _ENERGY_MONITOR_H
#define _ENERGY_MONITOR_H
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

//#define CONFIG_ONBOARD_CALLIBRATION y
//#define CONFIG_PF_CALCULATION
//#define CONFIG_REACTIVE_POWER
//#define CONFIG_CUSTOM_SATURATION_POINT
#define SATURATION_8_BIT 255
#define SATURATION_10_BIT 1023
#define SATURATION_12_BIT 4095
#define SATURATION_16_BIT 65535

#define MAX_SATURATION_POINT SATURATION_10_BIT
#define VOLTAGE_AVG_CAL_SAMPLE 1000
#define CURRENT_RMS_CAL_SAMPLE 1000
#define POWER_AVG_CAL_SAMPLE 1000

#define DEFAULT_V_OFFSET      525
#define DEFAULT_I_OFFSET      525

    //Signature for function raw value
    typedef uint16_t (*raw_value_function)(void);

    //Energy handle
    typedef struct
    {
        //raw value  functions for current and voltage
        raw_value_function get_voltage_raw_value;
        raw_value_function get_current_raw_value;

        //if waveform is inverse
        uint8_t is_inverse;

        //internal variable {advised not to use in application}
        uint16_t v_offset;
        uint16_t i_offset;
        uint16_t v_min;
        uint16_t v_max;
        uint16_t i_min;
        uint16_t i_max;
        int32_t p_integration;
        uint32_t i_integration;
        uint32_t v_integration;
#ifdef CONFIG_CUSTOM_SATURATION_POINT
        uint16_t v_sat;
        uint16_t i_sat;
#endif
        //average value for application
        int32_t p_avg;
        uint16_t v_avg;
        uint16_t i_avg;

        //relative counts for calculation
        uint16_t v_avg_cal_count;
        uint16_t i_avg_cal_count;
        uint16_t p_avg_cal_count;
    } energy_handle_t;

    void calculate_current_avg(energy_handle_t *phandle);
    void calculate_voltage_avg(energy_handle_t *phandle);
    void calculate_active_power(energy_handle_t *phandle);

#ifdef CONFIG_REACTIVE_POWER
    void calculate_reactive_power(energy_hanlde_t *phandle);
#endif
#ifdef CONFIG_PF_CALCULATION
    float get_power_factor(energy_hanlde_t *phandle);
#endif

    void energy_handle_init(energy_handle_t *phandle);

#ifdef __cplusplus
}
#endif

#endif