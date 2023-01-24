# Triac Controller Library
This is a triac controller library implemented in c for leading edge dimmer.

# Concepts
In this library one triac unit consist of:
- **Configuration**: static part that can not be changed after initialization
```
struct triac_ctrl_config {
  float start_trigger_angle;   /* !<the trigger angle that triac start with
                                  {180>angle>0} > */
  float trigger_pulse_lenght;  /* !<pulse lenght of trigger pulse ,expressed in
                                  form of angle> */
  float angle_increment_value; /* !< angle will be incremented ,depends on hf
                                  task frequency*/
  bool (*get_zc)(void); /* !<callback function for getting zero crossing */
  void (*set_trigger)(bool value); /* !<callback funtion for setting trigger */
};
```
- **Parameter**: Realtime variable can be modified while runnning.
```
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
```
Hence defination of triac unit stands
```
struct triac_unit {
  struct triac_ctrl_config *config;
  struct triac_ctrl_param *params;
};
```
