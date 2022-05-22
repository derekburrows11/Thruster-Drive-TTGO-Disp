
#pragma once

// VescUart.h originaly for Firmware x.xx
// Vesc code for Firmware 6.00 Test 29 (2022) is in C:\Users\user\Documents\PlatformIO\Projects\bldc, rev details in conf_general.h
// from http://github.com/vedderb/bldc - bldc FW rev dates:
//  6.00 <dev> started 18/1/2022
//  5.03 released 17/1/2022
//  5.02 released 11/1/2021
//  5.01 released 01/5/2020
//  5.00 released 27/4/2020

// for fault to string code see bldc/terminal.c
#include <VescUart.h>   // Library as a class derived from https://github.com/SolidGeek/VescUart, updated for FW5.3, 9th May 2022
// Older original VescUart library is from https://github.com/RollingGecko/VescUartControl - not updated for 5 years


void bldc_interface_process_packet(unsigned char *data, unsigned int len);


class VescUartExtra : public VescUart {
public:
// from Firmware 6.00.29
// structs from bldc_uart_comm_stm32f4_discovery\datatypes.h - date 29/12/2018
    mc_configuration mcconf;

    // Request then poll for response
    bool RequestVescValues(uint8_t canId = 0);
    bool RequestMCConfig(uint8_t canId = 0);
    bool Request(COMM_PACKET_ID command, uint8_t canId = 0);
    int CheckRxd();

    // Functions that wait for response
    int getVescValuesSize(uint8_t canId = 0);
    int getMCConfig(uint8_t canId = 0);
    int getMCConfig(mc_configuration& values);

    bool ProcessReadPacketMCConfig(uint8_t* message, mc_configuration& values, int len);

protected:
    uint8_t commandSent;
    uint8_t commandRxd;
    int sizeRxd;

	uint16_t lenMessage; // = 256;
	bool messageRead; // = false;
	bool messageOveflow; // = false;
	uint8_t messageReceived[256];
	uint16_t lenPayload; // = 0;
	const uint8_t* messagePayload; // = NULL;
	int timeout; // = millis() + _TIMEOUT; // Defining the timestamp for timeout (100ms before timeout)

};

