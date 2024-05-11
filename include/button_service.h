#ifndef ROLL4_BUTTON_SERVICE_H
#define ROLL4_BUTTON_SERVICE_h

#include "action_button.h"

void init_button_service();
int is_button_service_ready();
int register_button(struct action_button *button);
struct action_button *select_button_by_pin(int pin_num);
#endif