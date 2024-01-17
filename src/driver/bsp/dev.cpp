#include "assert.h"
#include "string.h"

#include "helpers.h"
#include "dev.h"
#include "clock.h"

struct RegisterItem
{
    DEV_ID id;
    Dev *handle;
    const char *name;
};

RegisterItem g_reg_tbl[DEV_ID_MAX];

int register_dev(DEV_ID id, Dev *handle, const char *name)
{
    assert(id && handle && name && name[0]);
    for (int i = 0;; i++)
    {
        if (g_reg_tbl[i].id == id)
            return -1;
        if (g_reg_tbl[i].id == DEV_ID_NONE)
        {
            g_reg_tbl[i].id = id;
            g_reg_tbl[i].name = name;
            g_reg_tbl[i].handle = handle;
            break;
        }
        assert(i != ARRAY_SIZE(g_reg_tbl));
    }
    return 0;
}

void unregister_dev(DEV_ID &id)
{
    if (id == DEV_ID_NONE)
        return;

    for (int i = 0;; i++)
    {
        assert(i != ARRAY_SIZE(g_reg_tbl));
        if (g_reg_tbl[i].id != id)
            continue;

        uint32_t left_sz = (ARRAY_SIZE(g_reg_tbl) - (i + 1)) * sizeof(g_reg_tbl[0]);
        if (left_sz > 0)
            memmove(g_reg_tbl + i, g_reg_tbl + i + 1, left_sz);
        RegisterItem tmp = {DEV_ID_NONE, NULL, NULL};
        g_reg_tbl[ARRAY_SIZE(g_reg_tbl) - 1] = tmp;
        break;
    }

    id = DEV_ID_NONE;
}

void unregister_dev(void *handle)
{
}

void unregister_dev(const char *name)
{
}

void Dev::poll(void)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(g_reg_tbl); i++)
    {
        if (g_reg_tbl[i].handle == NULL)
            return;
        g_reg_tbl[i].handle->event_poll();
    }
}

void reboot(void)
{
    mcu_reboot();
}
