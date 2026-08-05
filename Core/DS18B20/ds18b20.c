/**
 * @file ds18b20.c
 *
 * Created on 7/17/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Driver for the DS18B20 digital temperature sensor over One-Wire.
 */

#include "ds18b20.h"
#include "../OneWire/onewire.h"
#include "../Systim/systim.h"
#include "../CRC8/crc8.h"

static void handle_reset_seq_phase(DS18B20_Handle_t *hds);
static void handle_start_meas_phase(DS18B20_Handle_t *hds);
static void handle_wait_phase(DS18B20_Handle_t *hds);
static void handle_req_scratchpad_phase(DS18B20_Handle_t *hds);
static void handle_read_scratchpad_phase(DS18B20_Handle_t *hds);
static void handle_error_phase(DS18B20_Handle_t *hds);


DS18B20_Status_t DS18B20_Init(DS18B20_Handle_t *hds, OneWire_Handle_t *how) {
    if (hds == NULL) {
        return DS18B20_STATUS_NULL_ARG;
    }

    hds->how = how;

    hds->ctx.phase = DS18B20_PHASE_IDLE;
    hds->ctx.next_phase = DS18B20_PHASE_IDLE;
    hds->ctx.is_busy = false;
    hds->ctx.op_started = false;
    hds->ctx.last_error = DS18B20_ERROR_NONE;
    return DS18B20_STATUS_OK;
}

DS18B20_Status_t DS18B20_StartMeas_Async(DS18B20_Handle_t *hds) {
    if (hds == NULL) {
        return DS18B20_STATUS_NULL_ARG;
    }

    if (hds->ctx.is_busy) {
        return DS18B20_STATUS_BUSY;
    }

    hds->ctx.phase = DS18B20_PHASE_RESET_SEQ;
    hds->ctx.next_phase = DS18B20_PHASE_START_MEAS;
    hds->ctx.is_busy = true;
    return DS18B20_STATUS_OK;
}

bool DS18B20_IsDataReady(const DS18B20_Handle_t *hds) {
    if (hds == NULL) return false;

    return hds->ctx.phase == DS18B20_PHASE_DONE;
}

DS18B20_Status_t DS18B20_GetData(DS18B20_Handle_t *hds, float *temp) {
    if (hds == NULL || temp == NULL) {
        return DS18B20_STATUS_NULL_ARG;
    }

    int16_t raw_temp = 0;
    raw_temp = (int16_t) ((hds->scratchpad[1] << 8) | hds->scratchpad[0]);

    *temp = (float) raw_temp/16.0f;

    hds->ctx.phase = DS18B20_PHASE_IDLE;
    hds->ctx.is_busy = false;
    return DS18B20_STATUS_OK;
}

bool DS18B20_HasError(const DS18B20_Handle_t *hds) {
    if (hds == NULL) return false;

    return hds->ctx.phase == DS18B20_PHASE_ERROR;
}

DS18B20_Error_t DS18B20_GetError(const DS18B20_Handle_t *hds) {
    if (hds == NULL) return DS18B20_ERROR_NONE;

    return hds->ctx.last_error;
}

DS18B20_Status_t DS18B20_ClearError(DS18B20_Handle_t *hds) {
    if (hds == NULL) return DS18B20_STATUS_NULL_ARG;

    hds->ctx.last_error = DS18B20_ERROR_NONE;
    hds->ctx.phase = DS18B20_PHASE_IDLE;
    hds->ctx.is_busy = false;
    hds->ctx.op_started = false;
    OneWire_ClearError(hds->how);
    return DS18B20_STATUS_OK;
}

void DS18B20_Task(DS18B20_Handle_t *hds) {
    switch (hds->ctx.phase) {
        case DS18B20_PHASE_IDLE: // nothing to do
            break;
        case DS18B20_PHASE_RESET_SEQ: handle_reset_seq_phase(hds);
            break;
        case DS18B20_PHASE_START_MEAS: handle_start_meas_phase(hds);
            break;
        case DS18B20_PHASE_WAIT: handle_wait_phase(hds);
            break;
        case DS18B20_PHASE_REQ_SCRATCHPAD: handle_req_scratchpad_phase(hds);
            break;
        case DS18B20_PHASE_READ_SCRATCHPAD: handle_read_scratchpad_phase(hds);
            break;
        case DS18B20_PHASE_DONE: // nothing to do
            break;
        case DS18B20_PHASE_ERROR: handle_error_phase(hds);
            break;
        default:
            hds->ctx.phase = DS18B20_PHASE_ERROR;
            hds->ctx.last_error = DS18B20_ERROR_OTHER;
            break;
    }
}

static void handle_reset_seq_phase(DS18B20_Handle_t *hds) {
    if (!hds->ctx.op_started) {
        OneWire_Master_Reset(hds->how);
        hds->ctx.op_started = true;
        return;
    }

    const OneWire_OpStatus_t op_status = OneWire_GetOperationStatus(hds->how);

    if (op_status == OW_OPERATION_COMPLETE) {
        hds->ctx.op_started = false;
        hds->ctx.phase = hds->ctx.next_phase;
    } else if (op_status == OW_OPERATION_ERROR) {
        const OneWire_Error_t error = OneWire_GetError(hds->how);
        hds->ctx.phase = DS18B20_PHASE_ERROR;

        if (error == OW_ERROR_NO_DEVICE) {
            hds->ctx.last_error = DS18B20_ERROR_NO_DEVICE;
        } else {
            hds->ctx.last_error = DS18B20_ERROR_COMM_FAIL;
        }
    }
}

static void handle_start_meas_phase(DS18B20_Handle_t *hds) {
    if (!hds->ctx.op_started) {
        hds->cmd_buf = DS18B20_CONVERT_T;

        OneWire_Master_Transmit(hds->how, &hds->cmd_buf, 1);
        hds->ctx.op_started = true;
        return;
    }

    const OneWire_OpStatus_t op_status = OneWire_GetOperationStatus(hds->how);

    if (op_status == OW_OPERATION_COMPLETE) {
        hds->ctx.op_started = false;

        hds->ctx.phase = DS18B20_PHASE_WAIT;
        hds->ctx.now_us = Systim_GetUs();
        hds->ctx.deadline_us = DS18B20_12BIT_DELAY;
    } else if (op_status == OW_OPERATION_ERROR) {
        hds->ctx.phase = DS18B20_PHASE_ERROR;
        hds->ctx.last_error = DS18B20_ERROR_COMM_FAIL;
    }
}

static void handle_wait_phase(DS18B20_Handle_t *hds) {
    if (Systim_GetUs() - hds->ctx.now_us >= hds->ctx.deadline_us) {
        hds->ctx.phase = DS18B20_PHASE_RESET_SEQ;
        hds->ctx.next_phase = DS18B20_PHASE_REQ_SCRATCHPAD;
    }
}

static void handle_req_scratchpad_phase(DS18B20_Handle_t *hds) {
    if (!hds->ctx.op_started) {
        hds->cmd_buf = DS18B20_READ_SCRATCHPAD;

        OneWire_Master_Transmit(hds->how, &hds->cmd_buf, 1);
        hds->ctx.op_started = true;
        return;
    }

    const OneWire_OpStatus_t op_status = OneWire_GetOperationStatus(hds->how);

    if (op_status == OW_OPERATION_COMPLETE) {
        hds->ctx.op_started = false;
        hds->ctx.phase = DS18B20_PHASE_READ_SCRATCHPAD;
    } else if (op_status == OW_OPERATION_ERROR) {
        hds->ctx.phase = DS18B20_PHASE_ERROR;
        hds->ctx.last_error = DS18B20_ERROR_COMM_FAIL;
    }
}

static void handle_read_scratchpad_phase(DS18B20_Handle_t *hds) {
    if (!hds->ctx.op_started) {
        OneWire_Master_Receive(hds->how, hds->scratchpad, 9);
        hds->ctx.op_started = true;
        return;
    }

    const OneWire_OpStatus_t op_status = OneWire_GetOperationStatus(hds->how);

    if (op_status == OW_OPERATION_COMPLETE) {
        const uint8_t checksum = CRC8_Maxim(hds->scratchpad, 8);

        if (checksum == hds->scratchpad[8]) {
            hds->ctx.op_started = false;
            hds->ctx.phase = DS18B20_PHASE_DONE;
        } else {
            hds->ctx.phase = DS18B20_PHASE_ERROR;
            hds->ctx.last_error = DS18B20_ERROR_CRC;
        }
    } else if (op_status == OW_OPERATION_ERROR) {
        hds->ctx.phase = DS18B20_PHASE_ERROR;
        hds->ctx.last_error = DS18B20_ERROR_COMM_FAIL;
    }
}

static void handle_error_phase(DS18B20_Handle_t *hds) {
    hds->ctx.op_started = false;
}
