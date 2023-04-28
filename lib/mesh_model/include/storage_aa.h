/**
 * @defgroup settings Settings
 * @ingroup Api file_system_storage
 * @{
 */
#include <stdint.h>
#include <stddef.h>
#include <zephyr/settings/settings.h>

#ifndef _STORAGE_AA_H
#define _STORAGE__AA_H

enum ps_variables_id {
	RESET_COUNTER = 0x01,
	ACTIVATION_PWD,
};

extern uint8_t resetCounter;

extern struct k_work storageWork;

/* Call back from dynamic main tree handler */
int permanentStorageSet(const char *key, size_t lenRd, settings_read_cb readCb, void *cbArg);
int permanentStorageCommit(void);
int permanentStorageExport(int (*cb)(const char *name, const void *value, size_t val_len));

/* settings initialization */
int permanentStorageSettingsInit(void);

void saveOnFlash(uint8_t id);

#endif