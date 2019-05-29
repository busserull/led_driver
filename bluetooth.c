#include "bluetooth.h"
#include <string.h>

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_gatts.h"

#include "ubit.h"

#define CUSTOM_UUID_BASE {{\
	0x46, 0x58, 0x99, 0xa6, 0x43, 0x7f, 0x79, 0xb4,\
	0xcf, 0x4b, 0x1a, 0x2e, 0xaf, 0x10, 0xff, 0xd8\
}}

#define CUSTOM_UUID_SERVICE_LED 0xbabe
#define CUSTOM_UUID_CHAR_POWER 0xb00b

extern uint8_t __data_start__;

/**
 * @brief A number in the range 0..1000
 */
static uint8_t m_led_power[4] = {0};

static struct {
	uint16_t conn_handle;
	uint16_t service_handle;
    ble_gatts_char_handles_t led_power_handles;
} m_service;

uint32_t bluetooth_init(){
	uint32_t err_code = 0;

	err_code = sd_softdevice_enable(NULL, NULL);
	if(err_code){
		return err_code;
	}

	static ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));
	uint32_t app_ram_base = (uint32_t)&__data_start__;

	ble_enable_params.gap_enable_params.periph_conn_count = 1;
	ble_enable_params.common_enable_params.vs_uuid_count = 1;

	err_code = sd_ble_enable(&ble_enable_params, &app_ram_base);
	return err_code;
}

uint32_t bluetooth_gap_advertise_start(){
	uint32_t err_code = 0;

	static uint8_t adv_data[] = {
        10, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
        'P', 'l', 'a', 'n', 't', 'e', 'l', 'y', 's'
	};
	uint8_t adv_data_length = 11;

	err_code = sd_ble_gap_adv_data_set(
		adv_data, adv_data_length, NULL, 0
	);
	if(err_code){
		return err_code;
	}

	ble_gap_adv_params_t adv_params;
	memset(&adv_params, 0, sizeof(ble_gap_adv_params_t));
	adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	adv_params.interval = 0x12c;

	err_code = sd_ble_gap_adv_start(&adv_params);
	return err_code;
}

uint32_t bluetooth_gatts_start(){
	uint32_t err_code = 0;

    /* Add UUID base */

	ble_uuid128_t base_uuid = CUSTOM_UUID_BASE;

	ble_uuid_t plant_ligth_service_uuid;
	plant_ligth_service_uuid.uuid = CUSTOM_UUID_SERVICE_LED;

	err_code = sd_ble_uuid_vs_add(
		&base_uuid,
		&plant_ligth_service_uuid.type
	);


	err_code = sd_ble_gatts_service_add(
		BLE_GATTS_SRVC_TYPE_PRIMARY,
		&plant_ligth_service_uuid,
		&m_service.service_handle
	);

    /* Add power characteristic */

    ble_uuid_t power_uuid;
    power_uuid.uuid = CUSTOM_UUID_CHAR_POWER;

	err_code = sd_ble_uuid_vs_add(&base_uuid, &power_uuid.type);

    static uint8_t power_char_desc[] = {
        'P', 'l', 'a', 'n', 't', ' ',
        'l', 'i', 'g', 'h', 't', ' ',
        'p', 'o', 'w', 'e', 'r'
    };

	ble_gatts_char_md_t power_char_md;
	memset(&power_char_md, 0, sizeof(power_char_md));
	power_char_md.char_props.read = 1;
	power_char_md.char_props.write = 1;
	power_char_md.p_char_user_desc = power_char_desc;
	power_char_md.char_user_desc_max_size = 17;
	power_char_md.char_user_desc_size = 17;

	ble_gatts_attr_md_t power_attr_md;
	memset(&power_attr_md, 0, sizeof(power_attr_md));
	power_attr_md.read_perm.lv = 1;
	power_attr_md.read_perm.sm = 1;
	power_attr_md.write_perm.lv = 1;
	power_attr_md.write_perm.sm = 1;
	power_attr_md.vloc = BLE_GATTS_VLOC_USER;

	ble_gatts_attr_t power_attr;
	memset(&power_attr, 0, sizeof(power_attr));
	power_attr.p_uuid = &power_uuid;
	power_attr.p_attr_md = &power_attr_md;
	power_attr.init_len = 4;
	power_attr.max_len = 4;
	power_attr.p_value = m_led_power;

	err_code = sd_ble_gatts_characteristic_add(
		m_service.service_handle,
		&power_char_md,
		&power_attr,
		&m_service.led_power_handles
	);

	return err_code;
}

void bluetooth_serve_forever(){
	uint8_t ble_event_buffer[100] = {0};
	uint16_t ble_event_buffer_size = 100;

	while(1){
		/* if(m_matrix_attr_value != 0){ */
		/* 	ubit_led_matrix_turn_on(); */
		/* } */
		/* else{ */
		/* 	ubit_led_matrix_turn_off(); */
		/* } */

		while(
			sd_ble_evt_get(
				ble_event_buffer,
				&ble_event_buffer_size
			) != NRF_ERROR_NOT_FOUND
		)
        {

			ble_evt_t * p_ble_event = (ble_evt_t *)ble_event_buffer;
			uint16_t event_id = p_ble_event->header.evt_id;

			switch(event_id){
				case BLE_GAP_EVT_CONNECTED:
					m_service.conn_handle =
						p_ble_event->evt.gap_evt.conn_handle;

					sd_ble_gatts_sys_attr_set(
						m_service.conn_handle,
						0,
						0,
						0
					);
					break;

				case BLE_GAP_EVT_DISCONNECTED:
					m_service.conn_handle =
						BLE_CONN_HANDLE_INVALID;

					bluetooth_gap_advertise_start();
					break;

				default:
					break;
			}

			ble_event_buffer_size = 100;
		}
		ble_event_buffer_size = 100;

        if(m_service.conn_handle != BLE_CONN_HANDLE_INVALID){
            uint16_t notification_length = 2;

            ble_gatts_hvx_params_t hvx_params;
            memset(&hvx_params, 0, sizeof(hvx_params));
            hvx_params.handle =
                m_service.led_power_handles.value_handle;
            hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.p_len = &notification_length;
            hvx_params.p_data = m_led_power;

            sd_ble_gatts_hvx(
                m_service.conn_handle,
                &hvx_params
            );
        }

        uint32_t led_power = 0;
        led_power |= (m_led_power[0]);
        led_power |= (m_led_power[1] << 8);
        led_power |= (m_led_power[2] << 16);
        led_power |= (m_led_power[3] << 24);

        ubit_uart_print("led_power:%d\n\r", led_power);
	}
}
