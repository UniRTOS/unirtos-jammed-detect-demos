/*****************************************************************/ /**
* @file jammed_detect_demo.c
* @brief
* @author elmer.tang@quectel.com
* @date 2025-12-12
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-12-12 <td>1.0 <td>elmer.tang <td> Init
* </table>
**********************************************************************/
#include "qosa_def.h"
#include "qosa_log.h"
#include "qosa_sim.h"
#include "qosa_dev1.h"
#include "qosa_network.h"
#include "qosa_datacall.h"
#include "qosa_event_notify.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/** jammed detect demo task handle */
qosa_task_t g_jd_task = QOSA_NULL;

/**
 *
 * @struct unir_at_jamm_detect_msg_t
 * @brief The message reported by Jamm_Detect
 *
 */
typedef struct at_jamm_detect_msg
{
    qosa_uint32_t               cmd;                /*!< command id */
    qosa_nw_jamm_detect_event_t jamm_detect_status; /*!< command params cmd QOSA_EVENT_MODEM_SIM_INSERT_STATUS */

} unir_at_jamm_detect_msg_t;

/**
 * @brief jammed detect demo event callback function
 *
 * This function is used to handle jammed detect-related events
 *
 * @param[in] user_argv
 *          - User parameter, points to event ID
 *
 * @param[in] argv
 *          - Event data pointer, contains different structure data according to event type
 *
 * @return int
 *       - Returns 0 on success, -1 on failure
 */
static int jammed_detect_event_cb(void *user_argv, void *argv)
{
    qosa_notify_event_e        event_id = (qosa_ptr)user_argv;
    unir_at_jamm_detect_msg_t *msg = QOSA_NULL;

    /*Apply for a section of space to store the data of interference detection event reports. 
    The example reporting data type is provided. Customers can modify the logic according to their own needs.*/
    msg = qosa_malloc(sizeof(unir_at_jamm_detect_msg_t));

    if (!msg)
    {
        QLOGV("msg alloc failed!");
        return -1;
    }

    msg->cmd = event_id;
    QLOGI("nw msg id:%d", msg->cmd);
    switch (event_id)
    {
        case QOSA_EVENT_MODEM_JAMM_DETECT_STATUS: 
        {
            /*Interference detection result, jammdetectrslt ranges from 0 to 2. 
            1 :indicates no interference (NOJAMMING), and 2 :indicates interference has been detected (JAMMED).*/
            qosa_memcpy(&msg->jamm_detect_status, argv, sizeof(qosa_nw_jamm_detect_event_t));
            QLOGI("jammdetectrslt:%d", msg->jamm_detect_status.jammdetectrslt);
            break;
        }

        default:
            break;
    }
    //Since this is an example, no special handling is required. Therefore, the memory will be released once it is used up.
    qosa_free(msg);
    msg = QOSA_NULL;
    return 0;
}

/**
 * @brief jammed detect demo task main function
 *
 * This function serves as the main loop of the jammed detect demo task, implementing the following functions:
 * - Register jammed detect demo event callback functions
 * - Check SIM card status and jammed detect registration status
 * - Wait for jammed detect attachment, if failed then perform troubleshooting
 * - Periodically obtain and display jammed detect parameter information
 * - Support CFUN restart function to restore jammed detect connection
 *
 * @param[in] arg
 *          - Task parameter
 */
static void jd_demo_task(void *arg)
{
    int          ret = 0;
    qosa_uint8_t simid = 0;

    qosa_nw_jamm_detect_setting_param_t jdcfg[QOSA_NW_JDCFG_TYPE_ENUM_MAX] = {0};

    QOSA_UNUSED(arg);

    qosa_task_sleep_sec(3);

    // Register jamming detection status change event callback
    qosa_event_notify_register(QOSA_EVENT_MODEM_JAMM_DETECT_STATUS, jammed_detect_event_cb, (void *)QOSA_EVENT_MODEM_JAMM_DETECT_STATUS);

    /*Obtain the default values. Currently, the Eigencomm platform supports parameters and their corresponding value ranges.
    Mode[0,1],              Jamming detection algorithm control, value range is (0,1),it is mandatory for configuration. 0:Disabled 1:Enabled 
    rssi[-90,-30],          RSSI for the jamming detection result.
    rsrp[-140,44],          RSRP for the jamming detection result. 
    rsrq[-20,0],            RSRQ for the jamming detection result.
    period[0,10],           Time to trigger for Jamming Detection. 
    shakeperiod[0,60]       Periodical report time interval for Jamming Detection result. 0 indicates to report when the detected result changed */
    qosa_nw_get_jamm_detect_param(simid, &jdcfg[0]);

    QLOGI("jammed detect mode %d rsrp %d rsrq %d rssi %d shakeperiod %d period %d",
            jdcfg[QOSA_NW_JDCFG_TYPE_MODE].param.value,
            jdcfg[QOSA_NW_JDCFG_TYPE_RSRP].param.value,
            jdcfg[QOSA_NW_JDCFG_TYPE_RSRQ].param.value,
            jdcfg[QOSA_NW_JDCFG_TYPE_RSSI].param.value,
            jdcfg[QOSA_NW_JDCFG_TYPE_SHAKEPERIOD].param.value,
            jdcfg[QOSA_NW_JDCFG_TYPE_PERIOD].param.value);

    jdcfg[QOSA_NW_JDCFG_TYPE_MODE].param.value = 1;
    jdcfg[QOSA_NW_JDCFG_TYPE_URC].param.value = 1;

    qosa_nw_set_jamm_detect_param(simid, &jdcfg[0]);

    /*Set the configured interference detection parameters and pass in a custom callback function.
    (simid ：SIM card identifier, jdc_func ：interference detection parameters, cb ：callback function, ctx ：user context), or pass in NULL.*/
    ret = qosa_nw_set_jamm_detect_func(simid, &jdcfg[0], QOSA_NULL, QOSA_NULL);
    QLOGI("ret %d ", ret);
}

/**
 * @brief jammed detect demo initialization function
 *
 * This function is used to initialize jammed detect demo functionality, create jammed detect demo task
 *
 */
void unir_jammed_detect_demo_init(void)
{
    int err = 0;
    // Create jammed detect demo task
    err = qosa_task_create(&g_jd_task, 1024 * 4, QOSA_PRIORITY_NORMAL, "QJDDEMO", jd_demo_task, QOSA_NULL);
    if (err != QOSA_OK)
    {
        QLOGD("jd demo task create error");
        return;
    }
}
UNIRTOS_APP_EXPORT(333, "jammed_detect_demo", unir_jammed_detect_demo_init);
