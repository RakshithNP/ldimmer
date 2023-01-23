#include "energy_monitor.h"
#include <stdio.h>
#define __null_ptr ((void *)0)

void calculate_voltage_avg(energy_handle_t *phandle)
{
    uint16_t v_inst = 0;
    uint16_t v_mean = 0;
    uint16_t v_peak = 0;

    //Getting instanstanous value from hook function
    if (phandle->get_voltage_raw_value != __null_ptr)
    {
        v_inst = phandle->get_voltage_raw_value();
    }

    //Search for v_min and v_max over a period of time
    if (v_inst > (phandle->v_max))
    {
        phandle->v_max = v_inst;
    }
    if (v_inst < (phandle->v_min))
    {
        phandle->v_min = v_inst;
    }

    //Calculating V_mean
    if (v_inst >= phandle->v_offset)
    {
        v_mean = v_inst - (phandle->v_offset);
    }
    else
    {
        v_mean = (phandle->v_offset) - v_inst;
    }

    //intergrating or Summing V_mean
    phandle->v_integration += v_mean;

    //after VOLTAGE_AVG_CAL_SAMPLE time calculation of v_offset and v_avg
    if (phandle->v_avg_cal_count++ > VOLTAGE_AVG_CAL_SAMPLE)
    {
        phandle->v_avg = (phandle->v_integration / (phandle->v_avg_cal_count));
        v_peak = ((phandle->v_max) - (phandle->v_min)) / 2;
        phandle->v_offset = (phandle->v_min) + v_peak;
        phandle->v_min = MAX_SATURATION_POINT;
        phandle->v_max = 0;
        phandle->v_integration = 0;
        phandle->v_avg_cal_count = 0;
    }
}
void calculate_current_avg(energy_handle_t *phandle)
{
    uint16_t i_inst = 0;
    uint16_t i_mean = 0;
    uint16_t i_peak = 0;

    //Getting Raw value/ instenous value from hook function
    if (phandle->get_current_raw_value != __null_ptr)
    {
        i_inst = phandle->get_current_raw_value();
    }

    //Search for i_min and I_max over a period of time;
    if (i_inst > (phandle->i_max))
    {
        phandle->i_max = i_inst;
    }
    if (i_inst < (phandle->i_min))
    {
        phandle->i_min = i_inst;
    }

    //Getting I_mean
    if (i_inst >= (phandle->i_offset))
    {
        i_mean = i_inst - (phandle->i_offset);
    }
    else
    {
        i_mean = (phandle->i_offset) - i_inst;
    }

    //integrating or summing i_mean
    phandle->i_integration += i_mean;

    //calculating i_avg and i_offset
    if (phandle->i_avg_cal_count++ > CURRENT_RMS_CAL_SAMPLE)
    {
        phandle->i_avg = (phandle->i_integration) / (phandle->i_avg_cal_count);
        i_peak = ((phandle->i_max) - (phandle->i_min)) / 2;
        phandle->i_offset = (phandle->i_min) + i_peak;
        phandle->i_min = MAX_SATURATION_POINT;
        phandle->i_max = 0;
        phandle->i_integration = 0;
        phandle->i_avg_cal_count = 0;
    }
}
void calculate_active_power(energy_handle_t *phandle)
{
    uint16_t i_inst = 0;
    uint16_t v_inst = 0;
    uint16_t v_mean = 0;
    uint16_t i_mean = 0;
    uint32_t p_inst = 0;

    //Get raw instentenous value from hook function
    if ((phandle->get_voltage_raw_value != __null_ptr) && (phandle->get_current_raw_value != __null_ptr))
    {
        v_inst = phandle->get_voltage_raw_value();
        i_inst = phandle->get_current_raw_value();
    }
    else
    {
        phandle->p_avg = 0;
        phandle->p_integration = 0;
        return;
    }

    //calculating i_mean and v_mean
    if (v_inst >= (phandle->v_offset))
    {
        v_mean = v_inst - (phandle->v_offset);
    }
    else
    {
        v_mean = (phandle->v_offset) - v_inst;
    }

    if (i_inst >= (phandle->i_offset))
    {
        i_mean = (i_inst - (phandle->i_offset));
    }
    else
    {
        i_mean = ((phandle->i_offset) - i_inst);
    }

    //integrating or summing instantenous power;

    if ((v_inst > (phandle->v_offset)) && (i_inst > (phandle->i_offset)))
    {
        phandle->p_integration += i_mean * v_mean;
    }
    else if ((v_inst < (phandle->v_offset)) && (i_inst < (phandle->i_offset)))
    {
        phandle->p_integration += i_mean * v_mean;
    }
    else if ((v_inst > (phandle->v_offset)) && (i_inst < (phandle->i_offset)))
    {
        phandle->p_integration -= i_mean * v_mean;
    }
    else if ((v_inst < (phandle->v_offset)) && (i_inst > (phandle->i_offset)))
    {
        phandle->p_integration -= i_mean * v_mean;
    }
    else
    {
        phandle->p_integration += 0;
    }

    //calculating averaging readPowerSupply
    if (phandle->p_avg_cal_count++ > POWER_AVG_CAL_SAMPLE)
    {
        phandle->p_avg = (phandle->p_integration) / (phandle->p_avg_cal_count);
        phandle->p_avg_cal_count = 0;
        phandle->p_integration = 0;
        if (phandle->p_avg < 0)
        {
            if (phandle->is_inverse)
            {
                phandle->p_avg = (-1) * (phandle->p_avg);
            }
            else
            {
                phandle->p_avg = 0;
            }
        }
    }
}

#ifdef CONFIG_REACTIVE_POWER
void calculate_reactive_power(energy_hanlde_t *phandle)
{
    //Needs to be impemented;
}
#endif
#ifdef CONFIG_PF_CALCULATION
float get_power_factor(energy_hanlde_t *phandle)
{
    //needs to be implemented
}
#endif
void energy_handle_init(energy_handle_t *phandle)
{
    //Initalizing I values;
    phandle->i_max = 0;
    phandle->i_min = MAX_SATURATION_POINT;
    phandle->i_avg = 0;
    phandle->i_integration = 0;
    phandle->i_avg_cal_count = 0;
    phandle->i_offset = DEFAULT_I_OFFSET;

    //Initializing v values
    phandle->v_max = 0;
    phandle->v_min = MAX_SATURATION_POINT;
    phandle->v_avg = 0;
    phandle->v_integration = 0;
    phandle->v_avg_cal_count = 0;
    phandle->v_offset = DEFAULT_V_OFFSET;

    //Initializing P values

    phandle->p_avg = 0;
    phandle->p_avg_cal_count = 0;
    phandle->p_integration = 0;
}