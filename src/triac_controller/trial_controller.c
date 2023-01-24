#include "triac_controller.h"
#include <Arduino.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// local copies of triac
struct triac_unit *__triacs[MAX_TRIAC_NUM];
// This will be the count of triac present
static uint8_t triac_num;

bool zc_last_state;
uint8_t positive_level_count;
static bool filtered_zc_state(struct triac_unit *unit) {
  if (unit->config->get_zc == NULL) {
    return false;
  }
  bool ret_value = false;
  bool temp_state = unit->config->get_zc();

  if (zc_last_state) {
    if (temp_state) {
      if (positive_level_count++ > 3) {
        ret_value = true;
        positive_level_count = 0;
      }
    } else {
      positive_level_count = 0;
    }
  }
  zc_last_state = temp_state;
  return ret_value;
}

/**
 * @brief This function will debouce the zc
 *
 * To avoid false zero crossing senseing and
 * and proper angle claculation
 *
 * @param unit
 * @return true
 * if detected high on zc
 * @return false
 * if detected low on zc
 */
static bool debounce_zc_state(struct triac_unit *unit) {
  if (unit->config->get_zc == NULL) {
    return false;
  }
  bool ret_value = false;
  bool *array = unit->params->debounce_array;
  // Store zc value in the arrey at the index
  array[unit->params->zc_debounce_index] = unit->config->get_zc();

  // This array will hold only index (0-DEBOUNCE_ARREY_LENGHT)
  int index[DEBOUNCE_PONIT_NUM];
  // logic
  if (array[unit->params->zc_debounce_index]) {
    // find the check indexes of consecutive three point
    int j = 0;
    for (j = 0; j < DEBOUNCE_PONIT_NUM; j++) {
      if (unit->params->zc_debounce_index >= j) {
        index[j] = unit->params->zc_debounce_index - j;
      } else {
        index[j] =
            (DEBOUNCE_ARREY_LENGHT) - (j - (unit->params->zc_debounce_index));
      }
    }

    // if value at consecutive three index is high
    int i = 0;
    for (i = 0; i < DEBOUNCE_PONIT_NUM; i++) {
      if (!array[(index[i])]) {
        break;
      }
    }
    if (i >= 3) {
      ret_value = true;
    }
  }
  // check if end of the array
  if (unit->params->zc_debounce_index >= DEBOUNCE_ARREY_LENGHT - 1) {
    unit->params->zc_debounce_index = 0;
  } else {
    // increment index
    unit->params->zc_debounce_index += 1;
  }

  return ret_value;
}
/**
 * @brief This fucntion will clear all the instance parameter data
 *
 * @param unit
 */
static void clear_triac_unit_data(struct triac_unit *unit) {}
/**
 * @brief Init function for triac unit
 *
 * This will initialize triac_unit instance
 *
 * @param unit
 * @param config
 * @return int
 */
int triac_ctrl_init(struct triac_unit *unit) {
  // Null check of triac unit
  if (unit == NULL) {
    return -1;
  }
  // get config
  struct triac_ctrl_config *config = unit->config;
  // Check null config
  if (config == NULL) {
    return -2;
  }
  // if maximum triac allocated
  if (triac_num >= MAX_TRIAC_NUM) {
    return -3;
  }
  // check callback functions
  if ((config->get_zc == NULL) || (config->set_trigger == NULL)) {
    return -4;
  }
  // check start trigger angle
  if (config->start_trigger_angle < MIN_ALLOWE_TRIGGER_ANGLE) {
    config->start_trigger_angle = MIN_ALLOWE_TRIGGER_ANGLE;
  }
  if (config->start_trigger_angle > MAX_ALLOWED_TRIGGER_ANGLE) {
    config->start_trigger_angle = MAX_ALLOWED_TRIGGER_ANGLE;
  }
  // Save the pointer
  __triacs[triac_num] = unit;
  // Increment number
  triac_num += 1;
  return 0;
}
/**
 * @brief triac_ctrl_hf_task
 *
 * this function to be executed inside high frequency
 * timer
 *
 */
void triac_ctrl_hf_task_function(void) {
  for (int i = 0; i < triac_num; i++) {
    // Pointer NULL check
    if (__triacs[i] == NULL) {
      return;
    }
    // digitalWrite(6,0);
    //  Check if enable or Not
    if (!__triacs[i]->params->is_enable) {
      // Continue with the next instance
      continue;
    }
    // digitalWrite(6,1);
    //  Check zc function pointer
    if (__triacs[i]->config->get_zc == NULL) {
      // Continue with the next instance
      continue;
    }
    // digitalWrite(6,0);
    //  Get all the member
    struct triac_ctrl_config *config = __triacs[i]->config;
    struct triac_ctrl_param *params = __triacs[i]->params;
    // Debounce Zero crossing
    bool zc_state =config->get_zc();//filtered_zc_state(__triacs[i]);//debounce_zc_state(__triacs[i]);// filtered_zc_state(__triacs[i]);
    // Check for Rising edges
    if ((params->last_zc_state == false) && (zc_state)) {
      // TODO Rising edge routine
      params->phase_angle = 0;
      // Set flag of hot line present
      params->is_hot_line_present = true;
      // check if new angle hat to be set
      if (params->is_new_angle_queued) {
        // Set Queued angle yo trigger angle
        params->trigger_angle = params->queued_trigger_angle;
        // Reset Queue flag false;
        params->is_new_angle_queued = false;
      }
    }
    // Increse angle evry time
    if (params->is_hot_line_present) {
      params->phase_angle += (config->angle_increment_value);
    }
    // TODO Logic for hot line absent detection

    // check trigger angle for positive half cycle
    if (params->phase_angle < 180.0f) {
      if ((params->phase_angle > params->trigger_angle) &&
          (params->phase_angle <
           (params->trigger_angle + config->trigger_pulse_lenght))) {
        // if the is_trigger flag is not set
        if (!params->is_triggered) {
          // set the trigger
          config->set_trigger(true);
          // set the flag
          params->is_triggered = true;
        }
      } else {
        // Reseting the trigger signal
        if (params->is_triggered) {
          config->set_trigger(false);
          // reset the flag
          params->is_triggered = false;
        }
      }
    } else {
      float negative_cycle_angle = (params->phase_angle - 180.0f);
      if ((negative_cycle_angle > params->trigger_angle) &&
          (negative_cycle_angle <
           (params->trigger_angle + config->trigger_pulse_lenght))) {
        // if the is_trigger flag is not set
        if (!params->is_triggered) {
          // set the trigger
          config->set_trigger(true);
          // set the flag
          params->is_triggered = true;
        }
      } else {
        // Reseting the trigger signal
        if (params->is_triggered) {
          config->set_trigger(false);
          // reset the flag
          params->is_triggered = false;
        }
      }
    }
    // save zc state in last state
    params->last_zc_state = zc_state;
    // Save last phase angle
    params->last_phase_angle = params->phase_angle;
  }
}
/**
 * @brief Turn off triac control
 *
 * this function will disble the
 * triac instances
 *
 * @param unit
 * triac instance to be disbled
 */
void triac_ctrl_turn_off(struct triac_unit *unit) {
  // If already disable
  if (!unit->params->is_enable) {
    return;
  }
  // disable
  unit->params->is_enable = false;
  // Clear instance data
  clear_triac_unit_data(unit);
  // Set the triggr Low
  unit->config->set_trigger(false);
}
/**
 * @brief Turn on triac control
 *
 * @param unit
 */
void traic_ctrl_turn_on(struct triac_unit *unit) {
  // checcking is instanse is already enabled
  if (unit->params->is_enable) {
    // to avoid reseting the instance
    return;
  }
  // enabling unit
  unit->params->is_enable = true;
  // setting angle
  unit->params->trigger_angle = unit->config->start_trigger_angle;
}
/**
 * @brief This function will set trigger angle for triac
 *
 * @param unit
 *
 * @param angle
 *
 * This is the required angle for triac to be triggered
 * angle range will be 0 to 180 degree
 * to keep the sine wave chopping symitric
 *
 */
void triac_ctrl_set_trigger_angle(struct triac_unit *unit, float angle) {
  // Checking the the angle boundary
  if (angle > MAX_ALLOWED_TRIGGER_ANGLE || angle < MIN_ALLOWE_TRIGGER_ANGLE) {
    return;
  }
  // Set the trigger angle
  unit->params->queued_trigger_angle = angle;
  // set a flag of new angle
  unit->params->is_new_angle_queued = true;
}
/**
 * @brief this function will return trigger angle
 *
 * @param unit
 * @return float
 * trigger angle
 */
float triac_ctrl_get_trigger_angle(struct triac_unit *unit) {
  return (unit->params->trigger_angle);
}
