#include "bluetooth_conn.h"
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bluetooth_conn, LOG_LEVEL_INF);

static void on_connected(struct bt_conn *conn, uint8_t err) {
    LOG_INF("Connected : %d", err);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason) {
    LOG_INF("Disconnected : %d", reason);
}

static struct bt_conn_cb conn_cb = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};

void register_bt_conn_cb() {
    bt_conn_cb_register(&conn_cb);
    LOG_INF("Register Callback Complete");
}