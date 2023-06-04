#ifndef CHARGER_COMMANDS_H
#define CHARGER_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void batt_charge_enable(int argc, char **argv);
void batt_charge_disable(int argc, char **argv);
void batt_status(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif // CHARGER_COMMANDS_H
