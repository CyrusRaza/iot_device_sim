#include "unity.h"
#include "transport.h"
#include "transport_select.h"
#include "some_header.h"

#include <string.h>
#include <stdlib.h>



/* --------------------------------------------------------------------------
 * Test doubles / stubs for constructor functions
 * -------------------------------------------------------------------------- */

static int mqtt_constructor_called;
static int webs_constructor_called;

static char last_host[128];
static int last_port;
static char last_msg[128];
static char last_id[32];
static void *last_userdata;

static transport_type fake_mqtt_transport;
static transport_type fake_webs_transport;

transport_type* mqtt_constructor(char* host, int port, char* msg, char* id, void* userdata)
{
    mqtt_constructor_called++;

    strncpy(last_host, host, sizeof(last_host) - 1);
    last_host[sizeof(last_host) - 1] = '\0';

    last_port = port;

    strncpy(last_msg, msg, sizeof(last_msg) - 1);
    last_msg[sizeof(last_msg) - 1] = '\0';

    strncpy(last_id, id, sizeof(last_id) - 1);
    last_id[sizeof(last_id) - 1] = '\0';

    last_userdata = userdata;

    return &fake_mqtt_transport;
}

transport_type* webs_constructor(char* host, int port, char* msg, char* id, void* userdata)
{
    webs_constructor_called++;

    strncpy(last_host, host, sizeof(last_host) - 1);
    last_host[sizeof(last_host) - 1] = '\0';

    last_port = port;

    strncpy(last_msg, msg, sizeof(last_msg) - 1);
    last_msg[sizeof(last_msg) - 1] = '\0';

    strncpy(last_id, id, sizeof(last_id) - 1);
    last_id[sizeof(last_id) - 1] = '\0';

    last_userdata = userdata;

    return &fake_webs_transport;
}

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static void reset_constructor_state(void)
{
    mqtt_constructor_called = 0;
    webs_constructor_called = 0;

    last_host[0] = '\0';
    last_port = 0;
    last_msg[0] = '\0';
    last_id[0] = '\0';
    last_userdata = NULL;
}
/* --------------------------------------------------------------------------
 * Unity hooks
 * -------------------------------------------------------------------------- */

void setUp(void)
{
    reset_constructor_state();
}

void tearDown(void)
{
}

/* --------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

void test_transport_sel_returns_mqtt_transport_when_conf_transport_is_mqtt(void)
{
    config_pr conf;
    transport_type *result;
    int userdata_value = 123;

    memset(&conf, 0, sizeof(conf));
    conf.transport = MQTT;
    strcpy(conf.device_id, "dev1");

    result = transport_sel(conf, "broker.local", 1883, "hello", &userdata_value);

    TEST_ASSERT_EQUAL_PTR(&fake_mqtt_transport, result);
    TEST_ASSERT_EQUAL_INT(1, mqtt_constructor_called);
    TEST_ASSERT_EQUAL_INT(0, webs_constructor_called);

    TEST_ASSERT_EQUAL_STRING("broker.local", last_host);
    TEST_ASSERT_EQUAL_INT(1883, last_port);
    TEST_ASSERT_EQUAL_STRING("hello", last_msg);
    TEST_ASSERT_EQUAL_STRING("dev1", last_id);
    TEST_ASSERT_EQUAL_PTR(&userdata_value, last_userdata);
}

void test_transport_sel_returns_webs_transport_when_conf_transport_is_webs(void)
{
    config_pr conf;
    transport_type *result;
    int userdata_value = 456;

    memset(&conf, 0, sizeof(conf));
    conf.transport = WEBS;
    strcpy(conf.device_id, "dev2");

    result = transport_sel(conf, "server.local", 9001, "payload", &userdata_value);

    TEST_ASSERT_EQUAL_PTR(&fake_webs_transport, result);
    TEST_ASSERT_EQUAL_INT(0, mqtt_constructor_called);
    TEST_ASSERT_EQUAL_INT(1, webs_constructor_called);

    TEST_ASSERT_EQUAL_STRING("server.local", last_host);
    TEST_ASSERT_EQUAL_INT(9001, last_port);
    TEST_ASSERT_EQUAL_STRING("payload", last_msg);
    TEST_ASSERT_EQUAL_STRING("dev2", last_id);
    TEST_ASSERT_EQUAL_PTR(&userdata_value, last_userdata);
}

void test_transport_sel_returns_null_for_http_case_because_not_implemented(void)
{
    config_pr conf;
    transport_type *result;

    memset(&conf, 0, sizeof(conf));
    conf.transport = HTTP;
    strcpy(conf.device_id, "dev3");

    result = transport_sel(conf, "unused", 80, "ignored", NULL);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, mqtt_constructor_called);
    TEST_ASSERT_EQUAL_INT(0, webs_constructor_called);
}

void test_transport_sel_returns_null_for_invalid_transport_value(void)
{
    config_pr conf;
    transport_type *result;

    memset(&conf, 0, sizeof(conf));
    conf.transport = (enum types_transport)99;
    strcpy(conf.device_id, "dev4");

    result = transport_sel(conf, "unused", 1234, "ignored", NULL);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, mqtt_constructor_called);
    TEST_ASSERT_EQUAL_INT(0, webs_constructor_called);
}

void test_transport_sel_passes_device_id_to_constructor(void)
{
    config_pr conf;
    transport_type *result;

    memset(&conf, 0, sizeof(conf));
    conf.transport = MQTT;
    strcpy(conf.device_id, "abc123");

    result = transport_sel(conf, "host", 1883, "msg", NULL);

    TEST_ASSERT_EQUAL_PTR(&fake_mqtt_transport, result);
    TEST_ASSERT_EQUAL_STRING("abc123", last_id);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_transport_sel_returns_mqtt_transport_when_conf_transport_is_mqtt);
    RUN_TEST(test_transport_sel_returns_webs_transport_when_conf_transport_is_webs);
    RUN_TEST(test_transport_sel_returns_null_for_http_case_because_not_implemented);
    RUN_TEST(test_transport_sel_returns_null_for_invalid_transport_value);
    RUN_TEST(test_transport_sel_passes_device_id_to_constructor);

    return UNITY_END();
}