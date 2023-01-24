#ifndef _TRIAC_CONTROLLER_
#define _TRIAC_CONTROLLER_
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_TRIAC_NUM 2
#define DEBOUNCE_ARREY_LENGHT 8
#define DEBOUNCE_PONIT_NUM 3
#define MAX_ALLOWED_TRIGGER_ANGLE 135
#define MIN_ALLOWE_TRIGGER_ANGLE 35
/**
 * @brief triac controll configuration
 *
 * this will hold
 * 1.fucntion ponter to lower level function
 * 2.static configs
 *
 */
struct triac_ctrl_config {
  // float min_trigger_angle; /* !<Minimum allowed angle for trigger
  // {180>angle>0}> */ float max_trigger_angle; /* !<Maximum allowed amgle for
  // trigger {180>angle>0}> */
  float start_trigger_angle;   /* !<the trigger angle that triac start with
                                  {180>angle>0} > */
  float trigger_pulse_lenght;  /* !<pulse lenght of trigger pulse ,expressed in
                                  form of angle> */
  float angle_increment_value; /* !< angle will be incremented ,depends on hf
                                  task frequency*/
  bool (*get_zc)(void); /* !<callback function for getting zero crossing */
  void (*set_trigger)(bool value); /* !<callback funtion for setting trigger */
};
/**
 * @brief Triac controll parameter
 *
 * this structure will hold realtime
 * parameter
 *
 */
struct triac_ctrl_param {
  bool is_enable;
  volatile float phase_angle;
  float last_phase_angle;
  bool zc_state;
  bool last_zc_state;
  bool debounce_array[DEBOUNCE_ARREY_LENGHT];
  int zc_debounce_index;
  float trigger_angle;
  float queued_trigger_angle;
  bool  is_new_angle_queued;
  bool is_triggered;
  bool is_hot_line_present;
};
/**
 * @brief Tiac until
 *
 * logical representation of triac
 *
 */
struct triac_unit {
  struct triac_ctrl_config *config;
  struct triac_ctrl_param *params;
};

// API Functions
int triac_ctrl_init(struct triac_unit *unit);
void triac_ctrl_hf_task_function(void);
void triac_ctrl_turn_off(struct triac_unit *unit);
void traic_ctrl_turn_on(struct triac_unit *unit);
void triac_ctrl_set_trigger_angle(struct triac_unit *unit, float angle);
float triac_ctrl_get_trigger_angle(struct triac_unit *unit);
#ifdef __cplusplus
}
#endif
#endif
