#include "storage_aa.h"
#include "model_handler.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(storage, LOG_LEVEL_INF);

uint8_t resetCounter;
static uint8_t storageId;

static void saveResetCounter(void)
{
    int rc;
    rc = settings_save_one("ps/rc", &resetCounter, sizeof(resetCounter));
    if (rc)
    {
        LOG_DBG("fail (err %d)", rc);
    }
}

void settingsLoad(void)
{
    LOG_DBG("load all key-value pairs using registered handlers");
    settings_load();
}

static void saveActivationPwd(void)
{
    int rc;
    LOG_DBG("saveActivationPwd [%d] ", ctl->activation->pwd);
    rc = settings_save_one("ps/pwd", &ctl->activation->pwd, sizeof(ctl->activation->pwd));
    if (rc)
    {
        LOG_DBG("fail (err %d)", rc);
    }
}

static void storageWorkHandler(struct k_work *work)
{
    switch (storageId)
    {
    case RESET_COUNTER:
        saveResetCounter();
        break;
    case ACTIVATION_PWD:
        saveActivationPwd();
        break;
    }
}

K_WORK_DEFINE(storageWork, storageWorkHandler);

void saveOnFlash(uint8_t id)
{
    storageId = id;
    k_work_submit(&storageWork);
}

int permanentStorageSet(const char *key, size_t lenRd,
                        settings_read_cb readCb, void *cbArg)
{
    LOG_DBG("permanentStorageSet");
    ssize_t len = 0;
    int keyLen;
    const char *next;

    keyLen = settings_name_next(key, &next);

    if (!next)
    {
        if (!strncmp(key, "rc", keyLen))
        {
            len = readCb(cbArg, &resetCounter,
                         sizeof(resetCounter));
            LOG_DBG("<ps/rc> = %d", resetCounter);
        }

        if (!strncmp(key, "pwd", keyLen))
        {
            len = readCb(cbArg, &ctl->activation->pwd, sizeof(ctl->activation->pwd));
            LOG_DBG("<ps/pwd> = %d", ctl->activation->pwd);
        }

        return (len < 0) ? len : 0;
    }

    return -ENOENT;
}

int permanentStorageCommit(void)
{
    LOG_DBG("loading all settings <ps> handler is done");
    return 0;
}

int permanentStorageExport(int (*cb)(const char *name,
                                     const void *value, size_t val_len))
{
    LOG_DBG("export keys under <ps> handler");

    return 0;
}

static struct settings_handler psSettings = {
    .name = "ps",
    .h_get = NULL,
    .h_set = permanentStorageSet,
    .h_commit = permanentStorageCommit,
    .h_export = permanentStorageExport,
};

int permanentStorageSettingsInit(void)
{
    int err;

    err = settings_subsys_init();
    if (err)
    {
        LOG_DBG("settings_subsys_init failed (err %d)", err);
        return err;
    }

    err = settings_register(&psSettings);
    if (err)
    {
        LOG_DBG("ps_settings_register failed (err %d)", err);
        return err;
    }

    return 0;
}
