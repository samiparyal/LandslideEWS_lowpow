#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stm32wb0x.h"
#include "ble.h"
#include "gatt_profile.h"
#include "gap_profile.h"
#include "app_ble.h"
#include "stm32wb0x_hal_radio_timer.h"
#include "bleplat.h"
#include "nvm_db.h"
#include "blenvm.h"
#include "pka_manager.h"
#include "stm32_seq.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "landslide_service.h"
#include "imu.h"
#include "SEGGER_RTT.h"
#include "stm32_lpm.h"
#include "tilt_detector.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/**
 * Security parameters structure
 */
typedef struct
{
  uint8_t ioCapability;
  uint8_t mitm_mode;
  uint8_t bonding_mode;
  uint8_t encryptionKeySizeMin;
  uint8_t encryptionKeySizeMax;
  uint8_t initiateSecurity;
  /* USER CODE BEGIN tSecurityParams*/

  /* USER CODE END tSecurityParams */
}SecurityParams_t;

/**
 * Global context contains all BLE common variables.
 */
typedef struct
{
  SecurityParams_t bleSecurityParam;
  uint16_t gapServiceHandle;
  uint16_t devNameCharHandle;
  uint16_t appearanceCharHandle;
  uint16_t connectionHandle;
  /* USER CODE BEGIN BleGlobalContext_t*/

  /* USER CODE END BleGlobalContext_t */
}BleGlobalContext_t;

typedef struct
{
  BleGlobalContext_t BleApplicationContext_legacy;
  APP_BLE_ConnStatus_t Device_Connection_Status;
  /* USER CODE BEGIN PTD_1*/

  /* USER CODE END PTD_1 */
}BleApplicationContext_t;

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */


NO_INIT(uint32_t dyn_alloc_a[BLE_DYN_ALLOC_SIZE>>2]);

static BleApplicationContext_t bleAppCtx;

static const char a_GapDeviceName[] = {  'S', 'T', 'M', '3', '2', 'W', 'B', '0' }; /* Gap Device Name */

/**
 * Advertising Data
 */
uint8_t a_AdvData[] =
{
  2, AD_TYPE_FLAGS, FLAG_BIT_BR_EDR_NOT_SUPPORTED,
  14, AD_TYPE_COMPLETE_LOCAL_NAME, 'L', 'A', 'N', 'D', 'S', 'L', 'I', 'D', 'E', '_', 'E', 'W', 'S',  /* Complete name */

  3, AD_TYPE_16_BIT_SERV_UUID_CMPLT_LIST,
  0x01, 0xFF, //service uuid - 0xFF01

  /* Manufacturer Specific Data (0xFF)
     length + 1(type) + 2(company id) + 1(alarm) + f2(counter) + 1(src) = 7
  */
  7, 0xFF,
  0xFF, 0xFF,   /* company id (demo) */
  0x00,         /* alarm */
  0x00,      /* vel */
  0x00,          /* dph */
  0x00

};

/* USER CODE BEGIN PV */

/* Devices allowed to connect --- [[[[[ UNCOMMENT ON PROD ]]]]] */
typedef struct {
	uint8_t address_type;
	uint8_t address[6];
} AcceptedDevice_t;

static const List_Entry_t s_accepted_devices[] = {
	{ 0x01, { 0x47, 0x4C, 0x53, 0x54, 0x52, 0xC0 }},   // Gateway - ASCII for RTSLG
};

#define NUM_ACCEPTED_DEVICES (sizeof(s_accepted_devices)/sizeof(s_accepted_devices[0]))

static VTIMER_HandleType AlertTimerHandle;

/* Sampling is paced solely by the IMU's DRDY pulse on PB9 -- there is no poll
 * timer. The cadence is set by the ODR (imu_set_rate), so changing state
 * changes the wake rate with nothing to re-arm. Virtual timers were used here
 * before and are deliberately gone: arming one per sample put ~200 arms/s
 * against the radio scheduler's single hardware wakeup timer and froze the MCU.
 * Raw-IMU transmission is separately gated on BROADCAST_<state>_MS. */
static uint8_t g_in_alert_mode = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Connection_Interval,
                                      uint16_t Peripheral_Latency,
                                      uint16_t Supervision_Timeout);

static void gap_cmd_resp_wait(void);
static void gap_cmd_resp_release(void);

/* USER CODE BEGIN PFP */
static void APP_BLE_TiltPoll_Task(void);

/* USER CODE END PFP */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private functions ---------------------------------------------------------*/

/* USER CODE BEGIN PF */
/* Send cadence per state, used to gate the raw-IMU send below. */
static uint32_t APP_BLE_BroadcastIntervalMs(void)
{
    switch (tilt_detector_get_state())
    {
        case TILT_STATE_CRITICAL:    return BROADCAST_CRITICAL_MS;
        case TILT_STATE_WARNING:     return BROADCAST_WARNING_MS;
        /* Calibration streams every sample: 0 makes the gate below always pass,
           which says "no throttle" without naming a rate at all. */
        case TILT_STATE_CALIBRATING: return 0U;
        default:                     return BROADCAST_NORMAL_MS;
    }
}

static void APP_BLE_RawImuSend_Task(void)
{
    (void)Landslide_Send_Next_Pending();
}

/* DRDY arrives on two paths depending on power state: through GPIOB_IRQHandler
   while awake, and through PWR_ExitStopMode -> HAL_PWR_WKUP_IRQHandler after
   DEEPSTOP (the EXTI block is unpowered there). Both must schedule the same
   work or sampling stops after the first sleep. */
void HAL_GPIO_EXTI_Callback(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    if ((GPIOx != IMU_INT1_GPIO_Port) || (GPIO_Pin != IMU_INT1_Pin)) return;
    UTIL_SEQ_SetTask(1U << CFG_TASK_TILT_POLL_ID, CFG_SEQ_PRIO_0);
}

void HAL_PWR_WKUPx_Callback(uint32_t WakeupIOs)
{
    if ((WakeupIOs & PWR_WAKEUP_PB9) == 0U) return;
    UTIL_SEQ_SetTask(1U << CFG_TASK_TILT_POLL_ID, CFG_SEQ_PRIO_0);
}

/* USER CODE END PF */

/* Functions Definition ------------------------------------------------------*/
void ModulesInit(void)
{
  BLENVM_Init();
  if (PKAMGR_Init() == PKAMGR_ERROR)
  {
    Error_Handler();
  }
}

void BLE_Init(void)
{
  uint8_t role;
  uint8_t privacy_type = 0;
  tBleStatus ret;
  uint16_t gatt_service_changed_handle;
  uint16_t gap_dev_name_char_handle;
  uint16_t gap_appearance_char_handle;
  uint16_t gap_periph_pref_conn_param_char_handle;
  uint8_t bd_address[6] = {0};
  uint8_t bd_address_len= 6;
  uint16_t appearance = CFG_GAP_APPEARANCE;

  BLE_STACK_InitTypeDef BLE_STACK_InitParams = {
    .BLEStartRamAddress = (uint8_t*)dyn_alloc_a,
    .TotalBufferSize = BLE_DYN_ALLOC_SIZE,
    .NumAttrRecords = CFG_BLE_NUM_GATT_ATTRIBUTES,
    .MaxNumOfClientProcs = CFG_BLE_NUM_OF_CONCURRENT_GATT_CLIENT_PROC,
    .NumOfRadioTasks = CFG_BLE_NUM_RADIO_TASKS,
    .NumOfEATTChannels = CFG_BLE_NUM_EATT_CHANNELS,
    .NumBlockCount = CFG_BLE_MBLOCKS_COUNT,
    .ATT_MTU = CFG_BLE_ATT_MTU_MAX,
    .MaxConnEventLength = CFG_BLE_CONN_EVENT_LENGTH_MAX,
    .SleepClockAccuracy = CFG_BLE_SLEEP_CLOCK_ACCURACY,
    .NumOfAdvDataSet = CFG_BLE_NUM_ADV_SETS,
    .NumOfSubeventsPAwR = CFG_BLE_NUM_PAWR_SUBEVENTS,
    .MaxPAwRSubeventDataCount = CFG_BLE_PAWR_SUBEVENT_DATA_COUNT_MAX,
    .NumOfAuxScanSlots = CFG_BLE_NUM_AUX_SCAN_SLOTS,
    .FilterAcceptListSizeLog2 = CFG_BLE_FILTER_ACCEPT_LIST_SIZE_LOG2,
    .L2CAP_MPS = CFG_BLE_COC_MPS_MAX,
    .L2CAP_NumChannels = CFG_BLE_COC_NBR_MAX,
    .NumOfSyncSlots = CFG_BLE_NUM_SYNC_SLOTS,
    .CTE_MaxNumAntennaIDs = CFG_BLE_NUM_CTE_ANTENNA_IDS_MAX,
    .CTE_MaxNumIQSamples = CFG_BLE_NUM_CTE_IQ_SAMPLES_MAX,
    .NumOfSyncBIG = CFG_BLE_NUM_SYNC_BIG_MAX,
    .NumOfBrcBIG = CFG_BLE_NUM_BRC_BIG_MAX,
    .NumOfSyncBIS = CFG_BLE_NUM_SYNC_BIS_MAX,
    .NumOfBrcBIS = CFG_BLE_NUM_BRC_BIS_MAX,
    .NumOfCIG = CFG_BLE_NUM_CIG_MAX,
    .NumOfCIS = CFG_BLE_NUM_CIS_MAX,
    .isr0_fifo_size = CFG_BLE_ISR0_FIFO_SIZE,
    .isr1_fifo_size = CFG_BLE_ISR1_FIFO_SIZE,
    .user_fifo_size = CFG_BLE_USER_FIFO_SIZE
  };

  ret = BLE_STACK_Init(&BLE_STACK_InitParams);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("Error in BLE_STACK_Init() 0x%02x\r\n", ret);
    Error_Handler();
  }

  ret = hci_le_set_default_phy(0, HCI_TX_PHYS_LE_CODED_PREF, HCI_RX_PHYS_LE_CODED_PREF);
    if (ret != BLE_STATUS_SUCCESS) {
        SEGGER_RTT_printf(0, "hci_le_set_default_phy failed: 0x%02X\n", ret);
    }

#if (CFG_BD_ADDRESS_TYPE == HCI_ADDR_PUBLIC)

  bd_address[0] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x0000000000FF));
  bd_address[1] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x00000000FF00) >> 8);
  bd_address[2] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x000000FF0000) >> 16);
  bd_address[3] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x0000FF000000) >> 24);
  bd_address[4] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x00FF00000000) >> 32);
  bd_address[5] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0xFF0000000000) >> 40);
  (void)bd_address_len;

  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bd_address);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET\n");
  }
#endif

  ret = aci_hal_set_tx_power_level(0, CFG_TX_POWER);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_set_tx_power_level command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_set_tx_power_level command\n");
  }

  ret = aci_gatt_srv_profile_init(GATT_INIT_SERVICE_CHANGED_BIT, &gatt_service_changed_handle);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_srv_profile_init command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_srv_profile_init command\n");
  }

  role = 0U;
  role |= GAP_PERIPHERAL_ROLE;

#if CFG_BLE_PRIVACY_ENABLED
  privacy_type = 0x02;
#endif

  ret = aci_gap_init(privacy_type, CFG_BD_ADDRESS_TYPE);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_init command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_init command\n");
  }

  ret = aci_gap_profile_init(role, privacy_type,
                             &gap_dev_name_char_handle,
                             &gap_appearance_char_handle,
                             &gap_periph_pref_conn_param_char_handle);

#if (CFG_BD_ADDRESS_TYPE == HCI_ADDR_STATIC_RANDOM_ADDR)
  ret = aci_hal_read_config_data(CONFIG_DATA_STORED_STATIC_RANDOM_ADDRESS,
                                 &bd_address_len, bd_address);
  APP_DBG_MSG("  Static Random Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n",bd_address[5],bd_address[4],bd_address[3],bd_address[2],bd_address[1],bd_address[0]);
#elif (CFG_BD_ADDRESS_TYPE == HCI_ADDR_PUBLIC)
  APP_DBG_MSG("  Public Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n",bd_address[5],bd_address[4],bd_address[3],bd_address[2],bd_address[1],bd_address[0]);
#else
#error "Invalid CFG_BD_ADDRESS_TYPE"
#endif

  ret = Gap_profile_set_dev_name(0, sizeof(a_GapDeviceName), (uint8_t*)a_GapDeviceName);

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : Gap_profile_set_dev_name - Device Name, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: Gap_profile_set_dev_name - Device Name\n");
  }

  ret = Gap_profile_set_appearance(0, sizeof(appearance), (uint8_t*)&appearance);

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : Gap_profile_set_appearance - Appearance, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: Gap_profile_set_appearance - Appearance\n");
  }

  bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.ioCapability = CFG_IO_CAPABILITY;
  ret = aci_gap_set_io_capability(bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.ioCapability);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_set_io_capability command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_set_io_capability command\n");
  }

  bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.mitm_mode             = CFG_MITM_PROTECTION;
  bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin  = CFG_ENCRYPTION_KEY_SIZE_MIN;
  bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax  = CFG_ENCRYPTION_KEY_SIZE_MAX;
  bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.bonding_mode          = CFG_BONDING_MODE;

  ret = aci_gap_set_security_requirements(bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.bonding_mode,
                                               bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.mitm_mode,
                                               CFG_SC_SUPPORT,
                                               CFG_KEYPRESS_NOTIFICATION_SUPPORT,
                                               bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin,
                                               bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax,
                                               GAP_PAIRING_RESP_NONE);

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_set_security_requirements command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_set_security_requirements command\n");
  }

  if (bleAppCtx.BleApplicationContext_legacy.bleSecurityParam.bonding_mode)
  {
    ret = aci_gap_configure_filter_accept_and_resolving_list(0x01);
    SEGGER_RTT_printf(0, "[FilterListConfig] status=0x%02X\n", ret);
    if (ret != BLE_STATUS_SUCCESS)
    {
      APP_DBG_MSG("  Fail   : aci_gap_configure_filter_accept_and_resolving_list command, result: 0x%02X\n", ret);
    }
    else
    {
      APP_DBG_MSG("  Success: aci_gap_configure_filter_accept_and_resolving_list command\n");
    }
  }
  APP_DBG_MSG("==>> End BLE_Init function\n");

}

void BLEStack_Process_Schedule(void)
{
  UTIL_SEQ_SetTask( 1U << CFG_TASK_BLE_STACK, CFG_SEQ_PRIO_1);
}
static void BLEStack_Process(void)
{
  APP_DEBUG_SIGNAL_SET(APP_STACK_PROCESS);
  BLE_STACK_Tick();
  APP_DEBUG_SIGNAL_RESET(APP_STACK_PROCESS);
}

void VTimer_Process(void)
{
  HAL_RADIO_TIMER_Tick();
}

void VTimer_Process_Schedule(void)
{
  UTIL_SEQ_SetTask( 1U << CFG_TASK_VTIMER, CFG_SEQ_PRIO_0);
}
void NVM_Process(void)
{
  NVMDB_Tick();
}

void NVM_Process_Schedule(void)
{
  UTIL_SEQ_SetTask( 1U << CFG_TASK_NVM, CFG_SEQ_PRIO_1);
}

void HAL_RADIO_TIMER_TxRxWakeUpCallback(void)
{
  VTimer_Process_Schedule();
}

void HAL_RADIO_TIMER_CpuWakeUpCallback(void)
{
  VTimer_Process_Schedule();
}

void HAL_RADIO_TxRxCallback(uint32_t flags)
{
  BLE_STACK_RadioHandler(flags);
  VTimer_Process_Schedule();
  NVM_Process_Schedule();
}

void BLE_STACK_ProcessRequest(void)
{
  BLEStack_Process_Schedule();
}

/* Functions Definition ------------------------------------------------------*/
void APP_BLE_Init(void)
{
  UTIL_SEQ_RegTask(1U << CFG_TASK_BLE_STACK, UTIL_SEQ_RFU, BLEStack_Process);
  UTIL_SEQ_RegTask(1U << CFG_TASK_VTIMER, UTIL_SEQ_RFU, VTimer_Process);
  UTIL_SEQ_RegTask(1U << CFG_TASK_NVM, UTIL_SEQ_RFU, NVM_Process);
  UTIL_SEQ_RegTask(1U << CFG_TASK_ALARM_TIMEOUT_ID, UTIL_SEQ_RFU, APP_BLE_AlertTimeout);
  UTIL_SEQ_RegTask(1U << CFG_TASK_TILT_POLL_ID, UTIL_SEQ_RFU, APP_BLE_TiltPoll_Task);
  UTIL_SEQ_RegTask(1U << CFG_TASK_RAW_IMU_SEND_ID, UTIL_SEQ_RFU, APP_BLE_RawImuSend_Task);
  ModulesInit();

  BLE_Init();

  bleAppCtx.Device_Connection_Status = APP_BLE_IDLE;
  bleAppCtx.BleApplicationContext_legacy.connectionHandle = 0xFFFF;

  tBleStatus status = Landslide_Service_Init();
  if(status != BLE_STATUS_SUCCESS){
	  APP_DBG_MSG("ERROR: Landslide service init failed: 0x%02X\n", status);
  }

  /* Nothing to arm for sampling: imu_init() already set the ODR, so the sensor
     is pulsing INT1 and the first DRDY edge starts the chain on its own. */


  for (uint8_t i = 0; i < NUM_ACCEPTED_DEVICES; i++)
  {
	  tBleStatus filt_status = aci_gap_add_devices_to_filter_accept_and_resolving_list(
	                                0x01,
	                                0x00,
	                                NUM_ACCEPTED_DEVICES,
	                                (List_Entry_t*)s_accepted_devices);
	    SEGGER_RTT_printf(0, "[FilterList] status=0x%02X\n", filt_status);
  }

  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);

  return;
}

void BLEEVT_App_Notification(const hci_pckt *hci_pckt)
{
  tBleStatus ret = BLE_STATUS_ERROR;
  hci_event_pckt    *p_event_pckt;
  hci_le_meta_event *p_meta_evt;
  void *event_data;

  UNUSED(ret);

  if(hci_pckt->type != HCI_EVENT_PKT_TYPE && hci_pckt->type != HCI_EVENT_EXT_PKT_TYPE)
  {
    return;
  }

  p_event_pckt = (hci_event_pckt*)hci_pckt->data;

  if(hci_pckt->type == HCI_EVENT_PKT_TYPE){
    event_data = p_event_pckt->data;
  }
  else {
    hci_event_ext_pckt *p_event_pckt = (hci_event_ext_pckt*)hci_pckt->data;
    event_data = p_event_pckt->data;
  }

  switch (p_event_pckt->evt)
  {
  case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
    {
      hci_disconnection_complete_event_rp0 *p_disconnection_complete_event;
      p_disconnection_complete_event = (hci_disconnection_complete_event_rp0 *) p_event_pckt->data;

      if (p_disconnection_complete_event->Connection_Handle == bleAppCtx.BleApplicationContext_legacy.connectionHandle)
      {
        bleAppCtx.BleApplicationContext_legacy.connectionHandle = 0xFFFF;
        bleAppCtx.Device_Connection_Status = APP_BLE_IDLE;
        APP_DBG_MSG(">>== HCI_DISCONNECTION_COMPLETE_EVT_CODE\n");
        APP_DBG_MSG("     - Connection Handle:   0x%02X\n     - Reason:    0x%02X\n",
                    p_disconnection_complete_event->Connection_Handle,
                    p_disconnection_complete_event->Reason);

        Landslide_Service_Disconnection();
        g_in_alert_mode = 0;
        APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);
        SEGGER_RTT_printf(0, "Client Disconnected\n\n");
      }
      gap_cmd_resp_release();
    }
    break;

  case HCI_LE_META_EVT_CODE:
    {
      p_meta_evt = (hci_le_meta_event*) p_event_pckt->data;
      switch (p_meta_evt->subevent)
      {
      case HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE:
        {
          hci_le_connection_update_complete_event_rp0 *p_conn_update_complete;
          p_conn_update_complete = (hci_le_connection_update_complete_event_rp0 *) p_meta_evt->data;
          APP_DBG_MSG(">>== HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE\n");
          APP_DBG_MSG("     - Connection Interval:   %d.%02d ms\n     - Connection latency:    %d\n     - Supervision Timeout:   %d ms\n",
                      INT(p_conn_update_complete->Connection_Interval*1.25),
                      FRACTIONAL_2DIGITS(p_conn_update_complete->Connection_Interval*1.25),
                      p_conn_update_complete->Peripheral_Latency,
                      p_conn_update_complete->Supervision_Timeout*10);
          UNUSED(p_conn_update_complete);
        }
        break;
      case HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE:
        {
          hci_le_phy_update_complete_event_rp0 *p_le_phy_update_complete;
          p_le_phy_update_complete = (hci_le_phy_update_complete_event_rp0*)p_meta_evt->data;
          UNUSED(p_le_phy_update_complete);

          gap_cmd_resp_release();

          SEGGER_RTT_printf(0, "PHY update: status=0x%02X TX=%d RX=%d (3=Coded)\n",
                                  p_le_phy_update_complete->Status,
                                  p_le_phy_update_complete->TX_PHY,
                                  p_le_phy_update_complete->RX_PHY);
        }
        break;
      case HCI_LE_ENHANCED_CONNECTION_COMPLETE_SUBEVT_CODE:
        {
          hci_le_enhanced_connection_complete_event_rp0 *p_enhanced_conn_complete;
          p_enhanced_conn_complete = (hci_le_enhanced_connection_complete_event_rp0 *) p_meta_evt->data;

          connection_complete_event(p_enhanced_conn_complete->Status,
                                    p_enhanced_conn_complete->Connection_Handle,
                                    p_enhanced_conn_complete->Peer_Address_Type,
                                    p_enhanced_conn_complete->Peer_Address,
                                    p_enhanced_conn_complete->Connection_Interval,
                                    p_enhanced_conn_complete->Peripheral_Latency,
                                    p_enhanced_conn_complete->Supervision_Timeout);
        }
        break;
      case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE:
        {
          hci_le_connection_complete_event_rp0 *p_conn_complete;
          p_conn_complete = (hci_le_connection_complete_event_rp0 *) p_meta_evt->data;

          connection_complete_event(p_conn_complete->Status,
                                    p_conn_complete->Connection_Handle,
                                    p_conn_complete->Peer_Address_Type,
                                    p_conn_complete->Peer_Address,
                                    p_conn_complete->Connection_Interval,
                                    p_conn_complete->Peripheral_Latency,
                                    p_conn_complete->Supervision_Timeout);
        }
        break;

      default:
        break;
      }
    }
    break;

  case HCI_VENDOR_EVT_CODE:
    {
      aci_blecore_event *p_blecore_evt = (aci_blecore_event*) event_data;
      switch (p_blecore_evt->ecode)
      {
      case ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE:
        {
          aci_l2cap_connection_update_resp_event_rp0 *p_l2cap_conn_update_resp;
          p_l2cap_conn_update_resp = (aci_l2cap_connection_update_resp_event_rp0 *) p_blecore_evt->data;
          UNUSED(p_l2cap_conn_update_resp);
        }
        break;
      case ACI_GAP_PROC_COMPLETE_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_PROC_COMPLETE_VSEVT_CODE\n");
          aci_gap_proc_complete_event_rp0 *p_gap_proc_complete;
          p_gap_proc_complete = (aci_gap_proc_complete_event_rp0*) p_blecore_evt->data;
          UNUSED(p_gap_proc_complete);
        }
        break;
      case ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE:
        break;
      case ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE\n");
        }
        break;
      case ACI_GAP_PASSKEY_REQ_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_PASSKEY_REQ_VSEVT_CODE\n");

          ret = aci_gap_passkey_resp(bleAppCtx.BleApplicationContext_legacy.connectionHandle, CFG_FIXED_PIN);
          if (ret != BLE_STATUS_SUCCESS)
          {
            APP_DBG_MSG("==>> aci_gap_passkey_resp : Fail, reason: 0x%02X\n", ret);
          }
          else
          {
            APP_DBG_MSG("==>> aci_gap_passkey_resp : Success\n");
          }
        }
        break;
      case ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE\n");
          aci_gap_pairing_complete_event_rp0 *p_pairing_complete;
          p_pairing_complete = (aci_gap_pairing_complete_event_rp0*)p_blecore_evt->data;

          if (p_pairing_complete->Status != 0)
          {
            APP_DBG_MSG("     - Pairing KO\n     - Status: 0x%02X\n     - Reason: 0x%02X\n",
                        p_pairing_complete->Status, p_pairing_complete->Reason);
          }
          else
          {
            APP_DBG_MSG("     - Pairing Success\n");
          }
          APP_DBG_MSG("\n");
        }
        break;
      case ACI_GATT_SRV_READ_VSEVT_CODE :
        {
          APP_DBG_MSG(">>== ACI_GATT_SRV_READ_VSEVT_CODE\n");

          aci_gatt_srv_read_event_rp0    *p_read;
          p_read = (aci_gatt_srv_read_event_rp0*)p_blecore_evt->data;
          uint8_t error_code = BLE_ATT_ERR_INSUFF_AUTHORIZATION;

          APP_DBG_MSG("Handle 0x%04X\n",  p_read->Attribute_Handle);

          error_code =  BLE_ATT_ERR_NONE;

          aci_gatt_srv_resp(p_read->Connection_Handle,
                            p_read->CID,
                            p_read->Attribute_Handle,
                            error_code,
                            0,
                            NULL);
          break;
        }

      	case ACI_GATT_SRV_ATTRIBUTE_MODIFIED_VSEVT_CODE:
            {
          	  APP_DBG_MSG(">>== ACI_GATT_SRV_ATTRIBUTE_MODIFIED_VSEVT_CODE\n");
          	  aci_gatt_srv_attribute_modified_event_rp0 *p_attr_mod;
          	  p_attr_mod = (aci_gatt_srv_attribute_modified_event_rp0*)p_blecore_evt->data;

          	  Landslide_Service_AttributeModified(p_attr_mod->Attr_Handle,
          			  	  	  	  	  	  	  	  p_attr_mod->Attr_Data,
      											  p_attr_mod->Attr_Data_Length);
            }
           break;

      default:
        break;
      }
    }
    break;

  case HCI_HARDWARE_ERROR_EVT_CODE:
    {
      hci_hardware_error_event_rp0 *p_hci_hardware_error_event;
      p_hci_hardware_error_event = (hci_hardware_error_event_rp0*)p_event_pckt->data;

      if (p_hci_hardware_error_event->Hardware_Code <= 0x03)
      {
    	 SEGGER_RTT_printf(0, "!!! HCI HARDWARE ERROR code=0x%02X -- resetting\n",
    	                    p_hci_hardware_error_event->Hardware_Code);
        NVIC_SystemReset();
      }
    }
    break;

  default:
    break;
  }
}

static void connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Connection_Interval,
                                      uint16_t Peripheral_Latency,
                                      uint16_t Supervision_Timeout)
{
  if(Status != 0)
  {
    APP_DBG_MSG("==>> connection_complete_event Fail, Status: 0x%02X\n", Status);
    bleAppCtx.Device_Connection_Status = APP_BLE_IDLE;
    return;
  }

  APP_DBG_MSG(">>== hci_le_connection_complete_event - Connection handle: 0x%04X\n", Connection_Handle);
  APP_DBG_MSG("     - Connection established with @:%02x:%02x:%02x:%02x:%02x:%02x\n",
              Peer_Address[5],
              Peer_Address[4],
              Peer_Address[3],
              Peer_Address[2],
              Peer_Address[1],
              Peer_Address[0]);
  APP_DBG_MSG("     - Connection Interval:   %d.%02d ms\n     - Connection latency:    %d\n     - Supervision Timeout: %d ms\n",
              INT(Connection_Interval*1.25),
              FRACTIONAL_2DIGITS(Connection_Interval*1.25),
              Peripheral_Latency,
              Supervision_Timeout * 10
              );

  SEGGER_RTT_printf(0, "[Conn] interval=%d.%02d ms  latency=%d  supervision=%d ms\n",
                        INT(Connection_Interval*1.25),
                        FRACTIONAL_2DIGITS(Connection_Interval*1.25),
                        Peripheral_Latency,
                        Supervision_Timeout * 10);

  if (bleAppCtx.Device_Connection_Status == APP_BLE_LP_CONNECTING)
  {
    bleAppCtx.Device_Connection_Status = APP_BLE_CONNECTED_CLIENT;
  }
  else
  {
    bleAppCtx.Device_Connection_Status = APP_BLE_CONNECTED_SERVER;
  }
  bleAppCtx.BleApplicationContext_legacy.connectionHandle = Connection_Handle;

  Landslide_Service_ConnectionComplete(Connection_Handle);

  /* Send current state immediately to newly connected client */
  float vel = tilt_detector_get_rate_dph();
  float dev = tilt_detector_get_deviation();

  uint16_t vel_x100_16 = (uint16_t)(vel * 100.0f + 0.5f);
  uint16_t dev_x100_16 = (uint16_t)(dev * 100.0f + 0.5f);

  Landslide_Update_Alert_Status((uint8_t)tilt_detector_get_state(), tilt_detector_get_trigger_reason());
  Landslide_Update_Tilt_Data(dev_x100_16, vel_x100_16);

  tBleStatus ret = hci_le_set_phy(Connection_Handle,
                        0,                              // host has preference
                        HCI_TX_PHYS_LE_CODED_PREF,      // TX: coded
                        HCI_RX_PHYS_LE_CODED_PREF,      // RX: coded
                        2);                             // PHY_options=2 -> prefer S=8
   if (ret != BLE_STATUS_SUCCESS) {
 	  SEGGER_RTT_printf(0, "hci_le_set_phy failed: 0x%02X\n", ret);
   }
   else {
 	  SEGGER_RTT_printf(0, "==>> hci_le_set_phy Coded S=8 requested\n");
   }
}

APP_BLE_ConnStatus_t APP_BLE_Get_Server_Connection_Status(void)
{
  return bleAppCtx.Device_Connection_Status;
}

void APP_BLE_Procedure_Gap_General(ProcGapGeneralId_t ProcGapGeneralId)
{
  tBleStatus status;

  switch(ProcGapGeneralId)
  {
#if (CFG_BLE_CONTROLLER_2M_CODED_PHY_ENABLED == 1)
    case PROC_GAP_GEN_PHY_TOGGLE:
    {
      uint8_t phy_tx, phy_rx;

      status = hci_le_read_phy(bleAppCtx.BleApplicationContext_legacy.connectionHandle, &phy_tx, &phy_rx);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("hci_le_read_phy failure: reason=0x%02X\n",status);
      }
      else
      {
        APP_DBG_MSG("==>> hci_le_read_phy - Success\n");
        APP_DBG_MSG("==>> PHY Param  TX= %d, RX= %d\n", phy_tx, phy_rx);
        if ((phy_tx == HCI_TX_PHY_LE_2M) && (phy_rx == HCI_RX_PHY_LE_2M))
        {
          status = hci_le_set_phy(bleAppCtx.BleApplicationContext_legacy.connectionHandle, 0, HCI_TX_PHYS_LE_1M_PREF, HCI_RX_PHYS_LE_1M_PREF, 0);
          if (status == BLE_STATUS_SUCCESS)
          {
            gap_cmd_resp_wait();
          }
        }
        else
        {
          status = hci_le_set_phy(bleAppCtx.BleApplicationContext_legacy.connectionHandle, 0, HCI_TX_PHYS_LE_2M_PREF, HCI_RX_PHYS_LE_2M_PREF, 0);
          if (status == BLE_STATUS_SUCCESS)
          {
            gap_cmd_resp_wait();
          }
        }
      }
      break;
    }
#endif
    case PROC_GAP_GEN_CONN_TERMINATE:
    {
      status = aci_gap_terminate(bleAppCtx.BleApplicationContext_legacy.connectionHandle, BLE_ERROR_TERMINATED_REMOTE_USER);
      if (status == BLE_STATUS_SUCCESS)
      {
        gap_cmd_resp_wait();
      }
      break;
    }
    case PROC_GATT_EXCHANGE_CONFIG:
    {
      status = aci_gatt_clt_exchange_config(bleAppCtx.BleApplicationContext_legacy.connectionHandle);
      break;
    }
    default:
      break;
  }
  return;
}

void APP_BLE_Procedure_Gap_Peripheral(ProcGapPeripheralId_t ProcGapPeripheralId)
{
  tBleStatus status;
  uint32_t paramA = ADV_INTERVAL_MIN;
  uint32_t paramB = ADV_INTERVAL_MAX;
  uint32_t paramC, paramD;

  switch(ProcGapPeripheralId)
  {
    case PROC_GAP_PERIPH_ADVERTISE_START_FAST:
    {
      paramA = ADV_INTERVAL_MIN;
      paramB = ADV_INTERVAL_MAX;
      paramC = APP_BLE_ADV_FAST;
      break;
    }
    case PROC_GAP_PERIPH_ADVERTISE_START_LP:
    {
      paramA = ADV_LP_INTERVAL_MIN;
      paramB = ADV_LP_INTERVAL_MAX;
      paramC = APP_BLE_ADV_LP;
      break;
    }
    case PROC_GAP_PERIPH_ADVERTISE_STOP:
    {
      paramC = APP_BLE_IDLE;
      break;
    }
    case PROC_GAP_PERIPH_CONN_PARAM_UPDATE:
    {
      //paramA = CONN_INT_MS(2000);
      //paramB = CONN_INT_MS(4000);
      paramA = CONN_INT_MS(7.5);
      paramB = CONN_INT_MS(15);
      paramC = 0x0000;
      paramD = 0x01F4;
      break;
    }
    case PROC_GAP_PERIPH_CONN_TERMINATE:
    {
      status = aci_gap_terminate(bleAppCtx.BleApplicationContext_legacy.connectionHandle, 0x13);
      if (status == BLE_STATUS_SUCCESS)
      {
        gap_cmd_resp_wait();
      }
      break;
    }
    default:
      break;
  }

  switch(ProcGapPeripheralId)
  {
    case PROC_GAP_PERIPH_ADVERTISE_START_FAST:
    case PROC_GAP_PERIPH_ADVERTISE_START_LP:
    {
      Advertising_Set_Parameters_t Advertising_Set_Parameters = {0};

      status = aci_gap_set_advertising_configuration(0,
    		  	  	  	  	  	  	  	  	  	  	 GAP_MODE_NON_DISCOVERABLE,
                                                     ADV_TYPE,
                                                     paramA,
                                                     paramB,
                                                     HCI_ADV_CH_ALL,
                                                     0,
                                                     NULL,
													 ADV_FILTER,
                                                     0,
                                                     HCI_PHY_LE_1M,
                                                     0,
													 HCI_PHY_LE_CODED, /* Secondary advertising PHY (unused with legacy adv) */
                                                     0,
                                                     0);
      SEGGER_RTT_printf(0, "[AdvCfg] status=0x%02X\n", status);
      if (status == BLE_STATUS_SUCCESS)
      {
        bleAppCtx.Device_Connection_Status = (APP_BLE_ConnStatus_t)paramC;
      }

      status = aci_gap_set_advertising_data(0, ADV_COMPLETE_DATA, sizeof(a_AdvData), (uint8_t*) a_AdvData);
      status = aci_gap_set_advertising_enable(ENABLE, 1, &Advertising_Set_Parameters);
      break;
    }
    case PROC_GAP_PERIPH_ADVERTISE_STOP:
    {
      status = aci_gap_set_advertising_enable(DISABLE, 0, NULL);
      if (status == BLE_STATUS_SUCCESS)
      {
        bleAppCtx.Device_Connection_Status = (APP_BLE_ConnStatus_t)paramC;
      }
      break;
    }
    case PROC_GAP_PERIPH_CONN_PARAM_UPDATE:
    {
       status = aci_l2cap_connection_parameter_update_req(
                                                       bleAppCtx.BleApplicationContext_legacy.connectionHandle,
                                                       paramA,
                                                       paramB,
                                                       paramC,
                                                       paramD);
      break;
    }
    case PROC_GAP_PERIPH_SET_BROADCAST_MODE:
    {
      break;
    }
    default:
      break;
  }
  return;
}

/* USER CODE BEGIN FD*/

/* Sampling is virtual-timer driven; no IMU interrupt path. */

static void APP_BLE_TiltPoll_Task(void)
{
	static uint8_t was_calibrating = 1;
	static float last_valid_tilt_deg = 0.0f;

	if (imu_poll() != 0)
	{
		/* Drop this sample. The next DRDY edge re-triggers this task, so
		   nothing needs re-arming */
		return;
	}

	/* One scaling pass + one sqrtf for both outputs. On freefall the call
	   returns -1 and leaves tilt_deg alone, so it keeps the last good angle. */
	float tilt_deg = last_valid_tilt_deg;
	float gyro_mag = 0.0f;
	float acceleration_magnitude_g = 1.0f;

	if (imu_get_tilt_and_magnitude(&tilt_deg, &acceleration_magnitude_g) == 0)
	{
		last_valid_tilt_deg = tilt_deg;
	}

	TiltState_t state = tilt_detector_update(tilt_deg, gyro_mag,
	                                         acceleration_magnitude_g);

	float deviation = tilt_detector_get_deviation();
	float velocity  = tilt_detector_get_rate_dph();

	/* Buffer every sample into the raw-IMU ring; transmission is gated below at
	   the per-state BROADCAST_* cadence, which is the power budget. */
	{
		uint16_t dev_x100_buf = (uint16_t)(deviation * 100.0f + 0.5f);
		uint16_t vel_x100_buf = (uint16_t)(velocity  * 100.0f + 0.5f);
		uint8_t  gyro_was_off = (tilt_detector_get_state() == TILT_STATE_NORMAL) ? 1U : 0U;
		int16_t  accel[3] = {0}, gyro[3] = {0};
		imu_get_last_sample(accel, gyro);
		(void)Landslide_Buffer_Sample(dev_x100_buf, vel_x100_buf, accel, gyro, gyro_was_off);

		static uint32_t last_send_ms = 0;
		uint32_t now_ms = (uint32_t)((HAL_RADIO_TIMER_GetCurrentSysTime() * 10) >> 12);
		if ((now_ms - last_send_ms) >= APP_BLE_BroadcastIntervalMs())
		{
			last_send_ms = now_ms;
			UTIL_SEQ_SetTask(1U << CFG_TASK_RAW_IMU_SEND_ID, CFG_SEQ_PRIO_1);
		}
	}

	if(was_calibrating)
	{
		static uint8_t last_pct = 0;
		uint8_t pct = tilt_detector_get_calib_percent();
		if(pct != last_pct && (pct % 10) == 0)
		{
			last_pct = pct;
			SEGGER_RTT_printf(0, "[Calibrating] %d%%\n", pct);
			Landslide_Update_Alert_Status((uint8_t)TILT_STATE_CALIBRATING, TRIGGER_NONE);
		}
		if (pct == 100)
		{
			float baseline = tilt_detector_get_baseline();
			int bas_int  = (int)baseline;
			int bas_frac = (int)((baseline - (float)bas_int) * 100.0f);
			SEGGER_RTT_printf(0, "Calibration complete. Baseline = %d.%02d deg\n", bas_int, bas_frac);
		}
		was_calibrating = (state == TILT_STATE_CALIBRATING);
	}
	else
	{
	    int dev_int  = (int)deviation;
		int dev_frac = (int)((deviation - (float)dev_int) * 100.0f);
		int vel_int  = (int)velocity;
		int vel_frac = (int)((velocity - (float)vel_int) * 100.0f);

		static float last_rate = -1.0f;
		if (velocity != last_rate)
		{
			last_rate = velocity;
			SEGGER_RTT_printf(0, "[Rate] %d.%02d deg/h  (dev=%d.%02d)\n", vel_int, vel_frac, dev_int, dev_frac);
		}

		static TiltState_t last_printed_state = TILT_STATE_CALIBRATING;
		if(state != last_printed_state)
		{
		    last_printed_state = state;
		}

		static TiltState_t last_state = TILT_STATE_CALIBRATING;
		static uint8_t     last_src_deg     = 0;
		static uint8_t     last_vel_x100_b  = 0;
		static uint8_t     last_trigger     = 0;

		uint8_t cur_src_deg    = (uint8_t)deviation;
		float   cur_velocity   = tilt_detector_get_rate_dph();
		uint8_t cur_vel_x100_b = (uint8_t)(cur_velocity * 100.0f > 255.0f ? 255 : (uint8_t)(cur_velocity * 100.0f));
		uint8_t cur_trigger    = tilt_detector_get_trigger_reason();

		uint8_t data_changed = (state != last_state) ||
		                       (cur_src_deg    != last_src_deg)    ||
		                       (cur_vel_x100_b != last_vel_x100_b) ||
		                       (cur_trigger    != last_trigger);

		if (data_changed)
		{
			last_state      = state;
			last_src_deg     = cur_src_deg;
			last_vel_x100_b  = cur_vel_x100_b;
			last_trigger     = cur_trigger;

			APP_BLE_AlarmAdvUpdate((uint8_t)state, (uint8_t)deviation);
		}

		static uint64_t last_broadcast_at = 0;
		uint64_t now = HAL_RADIO_TIMER_GetCurrentSysTime();

		uint32_t interval;
		if (state == TILT_STATE_CRITICAL)      interval = BROADCAST_CRITICAL_MS;
		else if (state == TILT_STATE_WARNING)  interval = BROADCAST_WARNING_MS;
		else                                   interval = BROADCAST_NORMAL_MS;

		if (HAL_RADIO_TIMER_DiffSysTimeMs(now, last_broadcast_at) >= (int64_t)interval)
		{
			last_broadcast_at = now;
			APP_BLE_AlarmAdvUpdate((uint8_t)state, (uint8_t)deviation);
			SEGGER_RTT_printf(0, "[Broadcast] level=%d  dev=%d.%02d deg\n", (int)state, dev_int, dev_frac);
		}
	}

	(void)imu_set_rate(tilt_detector_get_rate());
}

void APP_BLE_AlarmAdvUpdate(uint8_t alarm, uint8_t src)
{
	float velocity     = tilt_detector_get_rate_dph();
	uint8_t vel_x100   = (uint8_t)(velocity * 100.0f > 255.0f ? 255 : (uint8_t)(velocity * 100.0f));

	a_AdvData[26] = alarm;
	a_AdvData[27] = src;
	a_AdvData[28] = vel_x100;
	a_AdvData[29] = tilt_detector_get_trigger_reason();

	tBleStatus status = aci_gap_set_advertising_data(0, ADV_COMPLETE_DATA, sizeof(a_AdvData), (uint8_t*)a_AdvData);

	if(status != BLE_STATUS_SUCCESS)
	{
		APP_DBG_MSG("aci_gap_set_advertising_data failed: 0x%02X\n", status);
	}

	if(bleAppCtx.BleApplicationContext_legacy.connectionHandle != 0xFFFF)
	{
		uint16_t dev_x100_16 = (uint16_t)(tilt_detector_get_deviation() * 100.0f + 0.5f);
		uint16_t vel_x100_16 = (uint16_t)(tilt_detector_get_rate_dph() * 100.0f + 0.5f);
		Landslide_Update_Tilt_Data(dev_x100_16, vel_x100_16);
		Landslide_Update_Alert_Status(alarm, tilt_detector_get_trigger_reason());
	}

	if (alarm == TILT_STATE_CRITICAL && !g_in_alert_mode)
	{
		g_in_alert_mode=1;
		if(bleAppCtx.BleApplicationContext_legacy.connectionHandle == 0xFFFF)
		{
			APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);
			APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);

			AlertTimerHandle.callback = APP_BLE_RadioTimer_Callback;
			HAL_RADIO_TIMER_StartVirtualTimer(&AlertTimerHandle, FAST_ADV_DURATION_MS);
		}
	}
	else if (alarm < TILT_STATE_CRITICAL && g_in_alert_mode)
	{
		if(bleAppCtx.BleApplicationContext_legacy.connectionHandle == 0xFFFF)
		{
			APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);
			APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);
		}
	}
}

void APP_BLE_AlertTimeout(void)
{
	if(g_in_alert_mode)
	{
			  g_in_alert_mode = 0;
			  if(bleAppCtx.BleApplicationContext_legacy.connectionHandle == 0xFFFF)
			 {
			  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);
			  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);
			  }
		}
}

void APP_BLE_RadioTimer_Callback(void *context)
{
  UTIL_SEQ_SetTask(1U << CFG_TASK_ALARM_TIMEOUT_ID, CFG_SEQ_PRIO_1);
}

/* USER CODE END FD*/

static void gap_cmd_resp_release(void)
{
  UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_PROC_GAP_COMPLETE);
  return;
}

static void gap_cmd_resp_wait(void)
{
  UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_PROC_GAP_COMPLETE);
  return;
}
