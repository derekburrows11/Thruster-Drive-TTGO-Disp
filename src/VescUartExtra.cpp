
//Library for VESC UART comms

//#include <buffer.h>
//#include <VecsUartControl/buffer.h>
#include "VescUartExtra.h"


//int fw_major;
//int fw_minor;


bool VescUartExtra::RequestVescValues(uint8_t canId) {
	return Request(COMM_GET_VALUES, canId);
}

bool VescUartExtra::RequestMCConfig(uint8_t canId) {
	return Request(COMM_GET_MCCONF, canId);
}

bool VescUartExtra::Request(COMM_PACKET_ID command, uint8_t canId) {
	int index = 0;
	int payloadSize = (canId == 0 ? 1 : 3);
	uint8_t payload[payloadSize];
	if (canId != 0) {
		payload[index++] = { COMM_FORWARD_CAN };
		payload[index++] = canId;
	}
	commandSent = command;
	payload[index++] = commandSent;
	packSendPayload(payload, payloadSize);
    sizeRxd = 0;
	messageRead = false;
	messageOveflow = false;
	commandRxd = -1;
	lenMessage = 0;
	messagePayload = 0;
	return true;
}

int VescUartExtra::CheckRxd() {
	while (serialPort->available()) {
		uint8_t rx = serialPort->read();
		if (sizeRxd < sizeof(messageReceived))
			messageReceived[sizeRxd++] = rx;
		else {
			sizeRxd++;
			messageOveflow = true;
		}
		if (sizeRxd == 3) {		// 2, or 3 to cover case 3
			switch (messageReceived[0]) {
			case 2:
				lenPayload = messageReceived[1];
				lenMessage = lenPayload + 5; //Payload size + 2 for sice + 3 for SRC and End.
				messagePayload = messageReceived + 2;
				break;
			case 3:
				//   Handling added by Derek 17/5/22
				lenPayload = ((uint16_t)messageReceived[1] << 8) + messageReceived[2];
				lenMessage = lenPayload + 6; //Payload size + 3 for size + 3 for SRC and End.
				messagePayload = messageReceived + 3;
				//log_d("message >256 bytes, payload %d", lenPayload);
				break;
			default:
				log_d("Invalid start byte");
			}
		}
		if (sizeRxd == lenMessage && rx == 3) {
			commandRxd = messagePayload[0];
			if (commandSent == commandRxd)
				messageRead = true;
			if (lenMessage < sizeof(messageReceived))
				messageReceived[lenMessage] = 0;	// can only do if lenMessage < 256, not if = 256
			break; // Exit if end of message is reached, even if there is still more data in the buffer.
		}
	}

if (messageOveflow) {
		log_d("Message >256 bytes overflowed, size: %d", counter);
		memcpy(payloadReceived, &messageReceived[3], 256-3);
		return lenPayload; 
	}
	if (messageRead == false) {
		log_d("Timed out");
		return 0;
	}


	int messageLength = receiveUartMessage(message);		// FW5.02 returns 73 bytes.  change - receive packet by polling for bytes
}

int VescUartExtra::Process() {

//log_d("receiveUartMessage returned len: %d", messageLength);
	if (messageLength > 55) {
		if (!processReadPacket(message))
			log_e("ProcessReadPacket failed on message length: %d", messageLength);
	} else
		log_e("Message length too short < 56: %d", messageLength);
//log_d("Done getVescValuesSize");
	return messageLength;

}


int VescUartExtra::getVescValuesSize(uint8_t canId) {
	if (debugPort)
		debugPort->println("Command: COMM_GET_VALUES " + String(canId));

	int32_t index = 0;
	int payloadSize = (canId == 0 ? 1 : 3);
	uint8_t payload[payloadSize];
	if (canId != 0) {
		payload[index++] = { COMM_FORWARD_CAN };
		payload[index++] = canId;
	}
	payload[index++] = { COMM_GET_VALUES };
//log_d("Requesting values");
	packSendPayload(payload, payloadSize);

	uint8_t message[256];
	int messageLength = receiveUartMessage(message);		// FW5.02 returns 73 bytes.  change - receive packet by polling for bytes
//log_d("receiveUartMessage returned len: %d", messageLength);
	if (messageLength > 55) {
		if (!processReadPacket(message))
			log_e("ProcessReadPacket failed on message length: %d", messageLength);
	} else
		log_e("Message length too short < 56: %d", messageLength);
//log_d("Done getVescValuesSize");
	return messageLength;
}



//  VescUart.h library does not have functions to read MC Config values
int VescUartExtra::getMCConfig(mc_configuration& values) {
	uint8_t command[1] = { COMM_GET_MCCONF };
//	log_d("Sending  COMM_GET_MCCONF");
	packSendPayload(command, 1);		// returns 464 = 0x1ca (458) + 6 bytes (first 3 for packet size + 3 for SRC and End) for firmware 5.2
//	log_d("Sent packSendPayload");	// ********* last message before crashing

//	delay(10); //needed, otherwise data is not read - original delay of 10ms
	//  delay(4);   // 6ms for 70 byte packet @115.2kb, 12ms @ 57.6.  Start read before buffer fills
  // int isizeMC = sizeof(mc_configuration);
//  uint8_t payload[256];
	uint8_t payload[512];
	int cntTotal = 0;

  if (0) {
	delay(10);
	int counter = 0;
	while (serialPort->available())
		payload[counter++] = serialPort->read();
	log_d("COMM_GET_MCCONF returned bytes after  10ms: %d - %.2x %.2x %.2x %.2x", counter, payload[0], payload[1], payload[2], payload[3]);
	cntTotal += counter;

	delay(20);
	counter = 0;
	while (serialPort->available())
		payload[counter++] = serialPort->read();
	log_d("COMM_GET_MCCONF returned bytes after +20ms: %d - %.2x %.2x %.2x %.2x", counter, payload[0], payload[1], payload[2], payload[3]);
	cntTotal += counter;

	delay(10);
	counter = 0;
	while (serialPort->available())
		payload[counter++] = serialPort->read();
	log_d("COMM_GET_MCCONF returned bytes after +10ms: %d - %.2x %.2x %.2x %.2x", counter, payload[0], payload[1], payload[2], payload[3]);
	cntTotal += counter;

	return cntTotal;
  }


	int lenPayload = receiveUartMessage(payload);
//	log_d("MCConfig lenPayload: %d ", lenPayload);
	if (lenPayload > 20) {
		if (ProcessReadPacketMCConfig(payload, values, lenPayload))  //returns true if sucessful
			log_e("ProcessReadPacketMCConfig failed on payload length: %d", lenPayload);
	} else
		log_e("Message length too short < 21: %d", lenPayload);

//	log_d("Done getMCConfig");
	return lenPayload;
}

bool VescUartExtra::ProcessReadPacketMCConfig(uint8_t* message, mc_configuration& values, int len) {
	COMM_PACKET_ID packetId;
	int32_t ind = 0;
	packetId = (COMM_PACKET_ID)message[0];
	message++;	//Eliminates the message id
	len--;
	switch (packetId) {
	case COMM_GET_MCCONF:
// ProcessReadPacket for FW 3.61 - FW 5.2
		if (len < 20)
			return false;
		values.l_current_max      = buffer_get_float16(message, 1e1, &ind);	// byte 0
		values.l_current_min      = buffer_get_float16(message, 1e1, &ind);
		values.l_in_current_max   = buffer_get_float32(message, 1e2, &ind);	// byte 4
		values.l_in_current_min   = buffer_get_float32(message, 1e2, &ind);
		values.l_max_erpm         = buffer_get_float32(message, 1e2, &ind);	// byte 12
	
    	values.comm_mode          = (mc_comm_mode)message[ind++];			// 
		values.foc_motor_r        = buffer_get_float32(message, 1e6, &ind);
		values.foc_motor_l        = buffer_get_float32(message, 1e6, &ind);

		return true;
		break;
	case COMM_GET_MCCONF_DEFAULT:

		return true;
		break;
	case COMM_SET_MCCONF:

		return true;
		break;
  	default:
		return false;
		break;
	}

}


// from Firmware 6.00.29
// structs from bldc_uart_comm_stm32f4_discovery\datatypes.h - date 29/12/2018

mc_configuration mcconf;

void bldc_interface_process_packet(unsigned char *data, unsigned int len) {
	if (!len)
		return;
//	int i = 0;
	unsigned char id = data[0];
	data++;
	len--;
	int ind = 0;

	switch (id) {
	case COMM_FW_VERSION:
		if (len == 2) {
			fw_major = data[ind++];
			fw_minor = data[ind++];
		} else {
			fw_major = -1;
			fw_minor = -1;
		}
		break;


	case COMM_GET_MCCONF:
	case COMM_GET_MCCONF_DEFAULT:
		mcconf.pwm_mode		= (mc_pwm_mode)data[ind++];
		mcconf.comm_mode	= (mc_comm_mode)data[ind++];
		mcconf.motor_type	= (mc_motor_type)data[ind++];
		mcconf.sensor_mode	= (mc_sensor_mode)data[ind++];

		mcconf.l_current_max 	= buffer_get_float32_auto(data, &ind);
		mcconf.l_current_min 	= buffer_get_float32_auto(data, &ind);
		mcconf.l_in_current_max = buffer_get_float32_auto(data, &ind);
		mcconf.l_in_current_min = buffer_get_float32_auto(data, &ind);
		mcconf.l_abs_current_max = buffer_get_float32_auto(data, &ind);
		mcconf.l_min_erpm 		= buffer_get_float32_auto(data, &ind);
		mcconf.l_max_erpm 		= buffer_get_float32_auto(data, &ind);
		mcconf.l_erpm_start 	= buffer_get_float32_auto(data, &ind);
		mcconf.l_max_erpm_fbrake = buffer_get_float32_auto(data, &ind);
		mcconf.l_max_erpm_fbrake_cc = buffer_get_float32_auto(data, &ind);
		mcconf.l_min_vin 		= buffer_get_float32_auto(data, &ind);
		mcconf.l_max_vin 		= buffer_get_float32_auto(data, &ind);
		mcconf.l_battery_cut_start = buffer_get_float32_auto(data, &ind);
		mcconf.l_battery_cut_end = buffer_get_float32_auto(data, &ind);
		mcconf.l_slow_abs_current = data[ind++];
		mcconf.l_temp_fet_start = buffer_get_float32_auto(data, &ind);
		mcconf.l_temp_fet_end 	= buffer_get_float32_auto(data, &ind);
		mcconf.l_temp_motor_start = buffer_get_float32_auto(data, &ind);
		mcconf.l_temp_motor_end = buffer_get_float32_auto(data, &ind);
		mcconf.l_temp_accel_dec = buffer_get_float32_auto(data, &ind);
		mcconf.l_min_duty 		= buffer_get_float32_auto(data, &ind);
		mcconf.l_max_duty 		= buffer_get_float32_auto(data, &ind);
		mcconf.l_watt_max 		= buffer_get_float32_auto(data, &ind);
		mcconf.l_watt_min 		= buffer_get_float32_auto(data, &ind);

		mcconf.lo_current_max = mcconf.l_current_max;
		mcconf.lo_current_min = mcconf.l_current_min;
		mcconf.lo_in_current_max = mcconf.l_in_current_max;
		mcconf.lo_in_current_min = mcconf.l_in_current_min;
		mcconf.lo_current_motor_max_now = mcconf.l_current_max;
		mcconf.lo_current_motor_min_now = mcconf.l_current_min;

		mcconf.sl_min_erpm = buffer_get_float32_auto(data, &ind);
		mcconf.sl_min_erpm_cycle_int_limit = buffer_get_float32_auto(data, &ind);
		mcconf.sl_max_fullbreak_current_dir_change = buffer_get_float32_auto(data, &ind);
		mcconf.sl_cycle_int_limit = buffer_get_float32_auto(data, &ind);
		mcconf.sl_phase_advance_at_br = buffer_get_float32_auto(data, &ind);
		mcconf.sl_cycle_int_rpm_br = buffer_get_float32_auto(data, &ind);
		mcconf.sl_bemf_coupling_k = buffer_get_float32_auto(data, &ind);

		memcpy(mcconf.hall_table, data + ind, 8);
		ind += 8;
		mcconf.hall_sl_erpm = buffer_get_float32_auto(data, &ind);

	// FOC
		mcconf.foc_current_kp = buffer_get_float32_auto(data, &ind);
		mcconf.foc_current_ki = buffer_get_float32_auto(data, &ind);
		mcconf.foc_f_zv = buffer_get_float32_auto(data, &ind);			// updated to foc_f_zv
		mcconf.foc_dt_us = buffer_get_float32_auto(data, &ind);
		mcconf.foc_encoder_inverted = data[ind++];
		mcconf.foc_encoder_offset = buffer_get_float32_auto(data, &ind);
		mcconf.foc_encoder_ratio = buffer_get_float32_auto(data, &ind);
		mcconf.foc_sensor_mode = (mc_foc_sensor_mode) data[ind++];
		mcconf.foc_pll_kp = buffer_get_float32_auto(data, &ind);
		mcconf.foc_pll_ki = buffer_get_float32_auto(data, &ind);
		mcconf.foc_motor_l = buffer_get_float32_auto(data, &ind);
		mcconf.foc_motor_r = buffer_get_float32_auto(data, &ind);
		mcconf.foc_motor_flux_linkage = buffer_get_float32_auto(data, &ind);
		mcconf.foc_observer_gain = buffer_get_float32_auto(data, &ind);
		mcconf.foc_observer_gain_slow = buffer_get_float32_auto(data, &ind);
		mcconf.foc_duty_dowmramp_kp = buffer_get_float32_auto(data, &ind);
		mcconf.foc_duty_dowmramp_ki = buffer_get_float32_auto(data, &ind);
		mcconf.foc_openloop_rpm = buffer_get_float32_auto(data, &ind);
		mcconf.foc_sl_openloop_hyst = buffer_get_float32_auto(data, &ind);
		mcconf.foc_sl_openloop_time = buffer_get_float32_auto(data, &ind);
//		mcconf.foc_sl_d_current_duty = buffer_get_float32_auto(data, &ind);		// current versions have four other foc_sl_openloop...
//		mcconf.foc_sl_d_current_factor = buffer_get_float32_auto(data, &ind);	// current versions have four other foc_sl_openloop...
		memcpy(mcconf.foc_hall_table, data + ind, 8);
		ind += 8;
		mcconf.foc_sl_erpm = buffer_get_float32_auto(data, &ind);
		mcconf.foc_sample_v0_v7 = data[ind++];
		mcconf.foc_sample_high_current = data[ind++];
		mcconf.foc_sat_comp = buffer_get_float32_auto(data, &ind);
		mcconf.foc_temp_comp = data[ind++];
		mcconf.foc_temp_comp_base_temp = buffer_get_float32_auto(data, &ind);
		mcconf.foc_current_filter_const = buffer_get_float32_auto(data, &ind);

		mcconf.s_pid_kp = buffer_get_float32_auto(data, &ind);
		mcconf.s_pid_ki = buffer_get_float32_auto(data, &ind);
		mcconf.s_pid_kd = buffer_get_float32_auto(data, &ind);
		mcconf.s_pid_kd_filter = buffer_get_float32_auto(data, &ind);
		mcconf.s_pid_min_erpm = buffer_get_float32_auto(data, &ind);
		mcconf.s_pid_allow_braking = data[ind++];

		mcconf.p_pid_kp = buffer_get_float32_auto(data, &ind);
		mcconf.p_pid_ki = buffer_get_float32_auto(data, &ind);
		mcconf.p_pid_kd = buffer_get_float32_auto(data, &ind);
		mcconf.p_pid_kd_filter = buffer_get_float32_auto(data, &ind);
		mcconf.p_pid_ang_div = buffer_get_float32_auto(data, &ind);

		mcconf.cc_startup_boost_duty = buffer_get_float32_auto(data, &ind);
		mcconf.cc_min_current = buffer_get_float32_auto(data, &ind);
		mcconf.cc_gain = buffer_get_float32_auto(data, &ind);
		mcconf.cc_ramp_step_max = buffer_get_float32_auto(data, &ind);

		mcconf.m_fault_stop_time_ms = buffer_get_int32(data, &ind);
		mcconf.m_duty_ramp_step = buffer_get_float32_auto(data, &ind);
		mcconf.m_current_backoff_gain = buffer_get_float32_auto(data, &ind);
		mcconf.m_encoder_counts = buffer_get_uint32(data, &ind);
		mcconf.m_sensor_port_mode = (sensor_port_mode) data[ind++];
		mcconf.m_invert_direction = data[ind++];
		mcconf.m_drv8301_oc_mode = (drv8301_oc_mode) data[ind++];
		mcconf.m_drv8301_oc_adj = data[ind++];
		mcconf.m_bldc_f_sw_min = buffer_get_float32_auto(data, &ind);
		mcconf.m_bldc_f_sw_max = buffer_get_float32_auto(data, &ind);
		mcconf.m_dc_f_sw = buffer_get_float32_auto(data, &ind);
		mcconf.m_ntc_motor_beta = buffer_get_float32_auto(data, &ind);
		mcconf.m_out_aux_mode = (out_aux_mode) data[ind++];

		break;

	default:
		break;

	}


}