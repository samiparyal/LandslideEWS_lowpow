#include "landslide_service.h"
#include "app_common.h"
#include <stdint.h>
#include "ble.h"
#include <string.h>
#include "SEGGER_RTT.h"
#include "ble_gatt.h"
#include "tilt_detector.h"
#include "stm32wb0x_hal_radio_timer.h"


//vars-camelCase, Type-PascalCase, Fns-Pascal_Snake_Case, Const/Macro- ALL_CAPS

//attribute offsets
#define CHARACTERISTIC_VALUE_HANDLE_OFFSET  (1U)
#define CHARACTERISTIC_CCCD_HANDLE_OFFSET  (2U)
#define CONNECTION_HANDLE_INVALID (0xFFFFU) //no connection yet

//context: stores the base handles (decl handles)
typedef struct{
	uint16_t serviceHandle;
	//characteristics decl handles
	uint16_t alertStatusCharDeclHandle;
	uint16_t tiltDataCharDeclHandle;

}LandslideGattContext_t;

static LandslideGattContext_t landslideGattCtx;
static uint16_t connectionHandle = CONNECTION_HANDLE_INVALID; //connection starting state

//CCCD (client characteristic config descriptor (state) (per connection)
static uint8_t alertStatusNotifyEnabled = 0U;
static uint8_t tiltDataNotifyEnabled    = 0U;

//characteristic values
static uint8_t alertStatusValue[2] = {0};
static uint8_t tiltDataValue[4] = {0};



//// GATT server definitions
static ble_gatt_val_buffer_def_t alertStatusValueBuffer =
{
    .op_flags = BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG,
    .val_len = 2U,
    .buffer_len = 2U,
    .buffer_p = alertStatusValue
};


static ble_gatt_val_buffer_def_t tiltDataValueBuffer =
{
    .op_flags = BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG,
    .val_len = 4U,
    .buffer_len = 4U,
    .buffer_p = tiltDataValue
};

static ble_gatt_chr_def_t landslideCharacteristics[2];
static ble_gatt_srv_def_t landslideServiceDefinition; //serv defn


//handle helpers
static inline uint16_t getAlertStatusValueHandle(void)
{ return(uint16_t)(landslideGattCtx.alertStatusCharDeclHandle + CHARACTERISTIC_VALUE_HANDLE_OFFSET); }

static inline uint16_t getAlertStatusCccdHandle(void)
{ return(uint16_t)(landslideGattCtx.alertStatusCharDeclHandle + CHARACTERISTIC_CCCD_HANDLE_OFFSET); }

//
static inline uint16_t getTiltDataValueHandle(void)
{ return(uint16_t)(landslideGattCtx.tiltDataCharDeclHandle + CHARACTERISTIC_VALUE_HANDLE_OFFSET); }

static inline uint16_t getTiltDataCccdHandle(void)
{ return(uint16_t)(landslideGattCtx.tiltDataCharDeclHandle + CHARACTERISTIC_CCCD_HANDLE_OFFSET); }

//



// CCCD declarations
BLE_GATT_SRV_CCCD_DECLARE(alertStatusCccd,
						  CFG_BLE_NUM_RADIO_TASKS, // num_of_links (1 connection)
                          BLE_GATT_SRV_CCCD_PERM_DEFAULT,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);

BLE_GATT_SRV_CCCD_DECLARE(tiltDataCccd,
						  CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_CCCD_PERM_DEFAULT,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);



//init landslide service and chars
tBleStatus Landslide_Service_Init(void)
{
	tBleStatus status;

	//reset context and runtime state
	memset(&landslideGattCtx, 0, sizeof(landslideGattCtx));

	connectionHandle = CONNECTION_HANDLE_INVALID;
	alertStatusNotifyEnabled  = 0U;
	tiltDataNotifyEnabled     = 0U;
	memset(alertStatusValue, 0, sizeof(alertStatusValue));
	memset(tiltDataValue, 0, sizeof(tiltDataValue));

	memset(&landslideServiceDefinition, 0, sizeof(landslideServiceDefinition));
	memset(landslideCharacteristics, 0, sizeof(landslideCharacteristics));

	//Motion descr_count: 0 - from debug traces
	//so lets try by explictly initializing CCCD descriptors

	//////

	//Characteristics0
	landslideCharacteristics[0].properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_NOTIFY;
	landslideCharacteristics[0].permissions = BLE_GATT_SRV_PERM_NONE;
	landslideCharacteristics[0].min_key_size = 0U;
	landslideCharacteristics[0].uuid = (ble_uuid_t)BLE_UUID_INIT_16(ALERT_STATUS_CHAR_UUID);
	landslideCharacteristics[0].val_buffer_p = &alertStatusValueBuffer;

	//add cccd using the declared cccd
	landslideCharacteristics[0].descrs.descr_count = 1;
	landslideCharacteristics[0].descrs.descrs_p = &alertStatusCccd_cccd;

	///

	//Characteristics1
	landslideCharacteristics[1].properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_NOTIFY;
	landslideCharacteristics[1].permissions = BLE_GATT_SRV_PERM_NONE;
	landslideCharacteristics[1].min_key_size = 0U;
	landslideCharacteristics[1].uuid = (ble_uuid_t)BLE_UUID_INIT_16(TILT_DATA_CHAR_UUID);
	landslideCharacteristics[1].val_buffer_p = &tiltDataValueBuffer;

	//add cccd using the declared cccd
	landslideCharacteristics[1].descrs.descr_count = 1;
	landslideCharacteristics[1].descrs.descrs_p = &tiltDataCccd_cccd;

	///


	//////

	//service defn
	landslideServiceDefinition.type = BLE_GATT_SRV_PRIMARY_SRV_TYPE;
	landslideServiceDefinition.uuid = (ble_uuid_t)BLE_UUID_INIT_16(LANDSLIDE_SERVICE_UUID);
	landslideServiceDefinition.chrs.chr_count = 2U;
	landslideServiceDefinition.chrs.chrs_p = landslideCharacteristics;
	landslideServiceDefinition.included_srv.incl_srv_count = 0U;
	landslideServiceDefinition.included_srv.included_srv_pp = NULL;


	//debug print
	SEGGER_RTT_printf(0, "\r\n=========================\r\n");
	SEGGER_RTT_printf(0, "AlertStatus properties:  0x%02X\r\n", landslideCharacteristics[0].properties);
	SEGGER_RTT_printf(0, "TiltData properties:  0x%02X\r\n", landslideCharacteristics[1].properties);
	SEGGER_RTT_printf(0, "AlertStatus descr_count: %d\r\n", landslideCharacteristics[0].descrs.descr_count);
	SEGGER_RTT_printf(0, "TiltData descr_count: %d\r\n", landslideCharacteristics[1].descrs.descr_count);
	SEGGER_RTT_printf(0, "=============================\r\n\r\n");


	//add service to gatt database
	status = aci_gatt_srv_add_service(&landslideServiceDefinition);
	if(status != BLE_STATUS_SUCCESS)
	{
		return status;
	}

	landslideGattCtx.serviceHandle = aci_gatt_srv_get_service_handle(&landslideServiceDefinition);
	landslideGattCtx.alertStatusCharDeclHandle = aci_gatt_srv_get_char_decl_handle(&landslideCharacteristics[0]);
	landslideGattCtx.tiltDataCharDeclHandle    = aci_gatt_srv_get_char_decl_handle(&landslideCharacteristics[1]);



	/* Inside Landslide_Service_Init after handles are fetched */
	SEGGER_RTT_printf(0, "\r\n--- GATT HANDLE DEBUG ---\r\n");
	SEGGER_RTT_printf(0, "Service Handle: 0x%04X\r\n", landslideGattCtx.serviceHandle);

	SEGGER_RTT_printf(0, "Alert Status:\r\n");
	SEGGER_RTT_printf(0, "  Decl:  0x%04X\r\n", landslideGattCtx.alertStatusCharDeclHandle);
	SEGGER_RTT_printf(0, "  Value: 0x%04X\r\n", getAlertStatusValueHandle());
	SEGGER_RTT_printf(0, "  CCCD:  0x%04X\r\n", getAlertStatusCccdHandle());

	SEGGER_RTT_printf(0, "Tilt Data:\r\n");
	SEGGER_RTT_printf(0, "  Decl:  0x%04X\r\n", landslideGattCtx.tiltDataCharDeclHandle);
	SEGGER_RTT_printf(0, "  Value: 0x%04X\r\n", getTiltDataValueHandle());
	SEGGER_RTT_printf(0, "  CCCD:  0x%04X\r\n", getTiltDataCccdHandle());


	return BLE_STATUS_SUCCESS;
}

void Landslide_Service_ConnectionComplete(uint16_t connHandle)
{
	connectionHandle = connHandle;

	//CCCD state - client controlled, reset each new connection
	alertStatusNotifyEnabled = 0U;
	tiltDataNotifyEnabled    = 0U;


}

void Landslide_Service_Disconnection(void)
{
	connectionHandle = CONNECTION_HANDLE_INVALID;
	alertStatusNotifyEnabled = 0U;
	tiltDataNotifyEnabled    = 0U;
}

// public api: handle cccd writes (attribute modified event)

void Landslide_Service_AttributeModified(uint16_t attr_handle, const uint8_t *data, uint8_t data_len)
{
	if((data==NULL) || (data_len < 2U))
	{
		return;
	}
	//cccd is 2 bytes
	// 0x0001 -> notif enabled
	// 0x0000 -> disabled

	uint16_t cccdValue = (uint16_t)(data[0] | ((uint16_t)data[1]<<8));
	uint8_t notifyEnabled = (cccdValue & 0x0001U)?1U:0U;

	//standard gatt table
	//0x0001 -> GAP
	//0x0002 -> Device Name Characteristic Declaration
	//0x0003 -> Device Name Value
	//0x0004 -> Appearance Characteristic OR Service Changed CCCD
	//0x0010 -> Landslide Service
	/*Motion Events:
	  Decl:  0x0011
	  Value: 0x0012
	  CCCD:  0x0013
	Wakeup Source:
	  Decl:  0x0014
	  Value: 0x0015
	  CCCD:  0x0016
	*/

	//value -- 0x0002 = bit 1 set == phone enabling indications on Service Changed/GATT database change

	if (attr_handle != getAlertStatusCccdHandle() && attr_handle != getTiltDataCccdHandle())
	{
		SEGGER_RTT_printf(0, "Handle: 0x%04X\r\n", attr_handle);
		return;
	}

	else if (attr_handle == getAlertStatusCccdHandle())
	{
		alertStatusNotifyEnabled = notifyEnabled;
		SEGGER_RTT_printf(0, ">>> AlertStatus Notifications %s\n\n", notifyEnabled ? "ENABLED" : "DISABLED");
	}

	else if (attr_handle == getTiltDataCccdHandle())
	{
		tiltDataNotifyEnabled = notifyEnabled;
		SEGGER_RTT_printf(0, ">>> TiltData Notifications %s\n\n", notifyEnabled ? "ENABLED" : "DISABLED");
	}


	else
	{
		SEGGER_RTT_printf(0, ">>> NO Match - Unknown Handle! \n\n");
	}

}

//update functions
tBleStatus Landslide_Update_Alert_Status(uint8_t alert_level, uint8_t trigger_reason)
{
    alertStatusValue[0] = alert_level;
    alertStatusValue[1] = trigger_reason;
    if ((connectionHandle != CONNECTION_HANDLE_INVALID) && (alertStatusNotifyEnabled != 0))
    {
        return aci_gatt_srv_notify(connectionHandle, BLE_GATT_UNENHANCED_ATT_L2CAP_CID,
                                   getAlertStatusValueHandle(),
                                   BLE_GATT_SRV_NOTIFY_FLAG_NOTIFICATION,
                                   2U, alertStatusValue);
    }
    return BLE_STATUS_SUCCESS;
}

tBleStatus Landslide_Update_Tilt_Data(uint16_t deviation_x100, uint16_t velocity_x100)
{
	tiltDataValue[0] = (uint8_t)(deviation_x100 & 0xFFU);
	tiltDataValue[1] = (uint8_t)(deviation_x100 >> 8);
	tiltDataValue[2] = (uint8_t)(velocity_x100 & 0xFFU);
	tiltDataValue[3] = (uint8_t)(velocity_x100 >> 8);

    if ((connectionHandle != CONNECTION_HANDLE_INVALID) && (tiltDataNotifyEnabled != 0))
    {
        return aci_gatt_srv_notify(connectionHandle, BLE_GATT_UNENHANCED_ATT_L2CAP_CID,
                                   getTiltDataValueHandle(),
                                   BLE_GATT_SRV_NOTIFY_FLAG_NOTIFICATION,
                                   4U, tiltDataValue);
    }
    return BLE_STATUS_SUCCESS;
}





