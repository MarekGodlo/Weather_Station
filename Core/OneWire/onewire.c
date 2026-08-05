/**
 * @file onewire_async.c
 *
 * Created on 7/22/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Provides the public API for controlling a One-Wire bus by bit-banging.
 */

#include "onewire.h"

static void one_wire_task(OneWire_Handle_t *how);

static void handle_idle_state(OneWire_Handle_t *how);
static void handle_reset_state(OneWire_Handle_t *how);
static void handle_rom_state(OneWire_Handle_t *how);
static void handle_write_state(OneWire_Handle_t *how);
static void handle_read_state(OneWire_Handle_t *how);
static void handle_error_state(OneWire_Handle_t *how);

static void handle_bit_write(OneWire_Handle_t *how);
static void handle_bit_read(OneWire_Handle_t *how);

static void set_timer_async(TIM_HandleTypeDef *htim, uint32_t us);
static void set_pin(const GPIO_Pin_t *pin);
static void reset_pin(const GPIO_Pin_t *pin);
static uint8_t read_pin(const GPIO_Pin_t *pin);

OneWire_Status_t OneWire_Init(OneWire_Handle_t *how, const OneWire_Config_t *config) {
    assert_param(how != NULL && config != NULL);

    if (how == NULL || config == NULL) {
        return OW_STATUS_NULL_ARG;
    }
    how->config = *config;

    how->ctx.is_busy = false;
    how->ctx.state = OW_STATE_IDLE;
    how->ctx.sub_state = 0;
    how->ctx.bit_bang_state = OW_BB_STATUS_OK;
    how->ctx.last_error = OW_ERROR_NONE;

    how->bit_index = 0;
    how->buf_index = 0;
    how->current_byte = 0;

    how->tx_len = 0;
    how->rx_len = 0;

    how->op_status = OW_OPERATION_RESET;
    return OW_STATUS_OK;
}

OneWire_Status_t OneWire_Master_Reset(OneWire_Handle_t *how) {
    assert_param(how != NULL);

    if (how == NULL) {
        return OW_STATUS_NULL_ARG;
    }
    if (how->ctx.is_busy) {
        return OW_STATUS_BUSY;
    }

    how->ctx.is_busy = true;
    how->ctx.bit_bang_state = OW_BB_STATUS_BUSY;
    how->ctx.state = OW_STATE_RESET;
    how->ctx.sub_state = 0;

    how->bit_index = 0;
    how->buf_index = 0;
    how->current_byte = 0;

    how->op_status = OW_OPERATION_BUSY;


    one_wire_task(how);
    return OW_STATUS_OK;
}

OneWire_Status_t OneWire_Master_Transmit(OneWire_Handle_t *how, const uint8_t *data, const uint8_t len) {
    assert_param(how != NULL && data != NULL);

    if (how == NULL || data == NULL) {
        return OW_STATUS_NULL_ARG;
    }
    if (how->ctx.is_busy) {
        return OW_STATUS_BUSY;
    }

    how->ctx.is_busy = true;
    how->ctx.bit_bang_state = OW_BB_STATUS_OK;
    how->ctx.state = OW_STATE_WRITE;
    how->ctx.sub_state = 0;

    how->bit_index = 0;
    how->buf_index = 0;
    how->current_byte = 0;

    how->tx_len = len;
    how->tx_buffer = data;

    how->op_status = OW_OPERATION_BUSY;

    one_wire_task(how);
    return OW_STATUS_OK;
}

OneWire_Status_t OneWire_Master_Receive(OneWire_Handle_t *how, uint8_t *data, const uint8_t len) {
    assert_param(how != NULL && data != NULL);

    if (how == NULL || data == NULL) {
        return OW_STATUS_NULL_ARG;
    }

    if (how->ctx.is_busy) {
        return OW_STATUS_BUSY;
    }

    how->ctx.is_busy = true;
    how->ctx.bit_bang_state = OW_BB_STATUS_BUSY;
    how->ctx.state = OW_STATE_READ;
    how->ctx.sub_state = 0;

    how->bit_index = 0;
    how->buf_index = 0;
    how->current_byte = 0;

    how->rx_len = len;
    how->rx_buffer = data;

    how->op_status = OW_OPERATION_BUSY;

    one_wire_task(how);
    return OW_STATUS_OK;
}

OneWire_State_t OneWire_GetState(OneWire_Handle_t *how) {
    assert_param(how != NULL);
    return how->ctx.state;
}

OneWire_OpStatus_t OneWire_GetOperationStatus(OneWire_Handle_t *how) {
    assert_param(how != NULL);
    return how->op_status;
}

OneWire_Error_t OneWire_GetError(OneWire_Handle_t *how) {
    assert_param(how != NULL);
    return how->ctx.last_error;
}

void OneWire_ClearError(OneWire_Handle_t *how) {
    assert_param(how != NULL);

    how->ctx.last_error = OW_ERROR_NONE;
    how->ctx.state = OW_STATE_IDLE;
}

void OneWire_OnTimerCplt(OneWire_Handle_t *how) {
    assert_param(how != NULL);
    one_wire_task(how);
}

static void one_wire_task(OneWire_Handle_t *how) {
    switch (how->ctx.state) {
        case OW_STATE_IDLE: handle_idle_state(how);
            break;
        case OW_STATE_RESET: handle_reset_state(how);
            break;
        case OW_STATE_ROM: handle_rom_state(how);
            break;
        case OW_STATE_WRITE: handle_write_state(how);
            break;
        case OW_STATE_READ: handle_read_state(how);
            break;
        case OW_STATE_ERROR: handle_error_state(how);
            break;
        default:
            how->ctx.last_error = OW_ERROR_INVALID_STATE;
            how->ctx.state = OW_STATE_ERROR;
            handle_error_state(how);
            break;
    }
}

static void handle_idle_state(OneWire_Handle_t *how) {
    HAL_TIM_Base_Stop_IT(how->config.htim);
    how->ctx.is_busy = false;
}

static void handle_reset_state(OneWire_Handle_t *how) {
    switch (how->ctx.sub_state) {
        case 0:
            reset_pin(how->config.data_pin);
            set_timer_async(how->config.htim, 480);

            how->ctx.sub_state = 1;
            break;
        case 1:
            set_pin(how->config.data_pin);
            set_timer_async(how->config.htim, 70);

            how->ctx.sub_state = 2;
            break;
        case 2:
            const int8_t sample = read_pin(how->config.data_pin);
            set_timer_async(how->config.htim, 410);

            if (!sample) {
                how->ctx.sub_state = 0;
                how->ctx.state = OW_STATE_ROM;
            } else {
                how->ctx.state = OW_STATE_ERROR;
                how->ctx.last_error = OW_ERROR_NO_DEVICE;
            }
            break;
        default:
            how->ctx.state = OW_STATE_ERROR;
            how->ctx.last_error = OW_ERROR_INVALID_SUB_STATE;
            break;
    }
}

static void handle_rom_state(OneWire_Handle_t *how) {
    if (how->config.bus_mode == OW_ROM_MODE_SKIP) {
        how->current_byte = OW_SKIP_ROM;
    } else {
        how->ctx.state = OW_STATE_ERROR;
        how->ctx.last_error = OW_ERROR_INVALID_MODE;
        handle_error_state(how);
    }

    if (how->ctx.bit_bang_state == OW_BB_STATUS_OK) {
        how->buf_index = 0;
        how->ctx.state = OW_STATE_IDLE;
        how->op_status = OW_OPERATION_COMPLETE;
        how->ctx.is_busy = false;
        return;
    }

    handle_bit_write(how);
}


static void handle_write_state(OneWire_Handle_t *how) {
    if (how->buf_index >= how->tx_len && how->ctx.bit_bang_state == OW_BB_STATUS_OK) {
        how->buf_index = 0;
        how->ctx.state = OW_STATE_IDLE;
        how->op_status = OW_OPERATION_COMPLETE;
        how->ctx.is_busy = false;
        return;
    }

    if (how->ctx.bit_bang_state == OW_BB_STATUS_OK) {
        how->current_byte = how->tx_buffer[how->buf_index++];
    }

    handle_bit_write(how);
}

static void handle_read_state(OneWire_Handle_t *how) {
    if (how->ctx.bit_bang_state == OW_BB_STATUS_OK) {
        how->rx_buffer[how->buf_index++] = how->current_byte;
        how->current_byte = 0;
    }

    if (how->buf_index >= how->rx_len) {
        how->buf_index = 0;
        how->ctx.state = OW_STATE_IDLE;
        how->op_status = OW_OPERATION_COMPLETE;
        how->ctx.is_busy = false;
        return;
    }

    handle_bit_read(how);
}

static void handle_error_state(OneWire_Handle_t *how) {
    how->ctx.sub_state = 0;
    how->ctx.is_busy = false;
    how->op_status = OW_OPERATION_ERROR;
    how->bit_index = 0;
    how->buf_index = 0;
    how->current_byte = 0;
}

static void handle_bit_write(OneWire_Handle_t *how) {
    switch (how->ctx.sub_state) {
        case 0: {
            how->ctx.bit_bang_state = OW_BB_STATUS_BUSY;
            const uint8_t bit = how->current_byte >> how->bit_index & 0x01;

            reset_pin(how->config.data_pin);
            set_timer_async(how->config.htim, bit ? 6 : 60);

            how->ctx.sub_state = 1;
            break;
        }
        case 1: {
            const uint8_t bit = (how->current_byte >> how->bit_index) & 0x01;

            set_pin(how->config.data_pin);
            set_timer_async(how->config.htim, bit ? 64 : 10);

            how->ctx.sub_state = 0;
            how->bit_index++;

            if (how->bit_index >= 8) {
                how->bit_index = 0;
                how->ctx.bit_bang_state = OW_BB_STATUS_OK;
            }
            break;
        }
        default:
            how->ctx.state = OW_STATE_ERROR;
            how->ctx.last_error = OW_ERROR_INVALID_SUB_STATE;
            break;
    }
}

static void handle_bit_read(OneWire_Handle_t *how) {
    switch (how->ctx.sub_state) {
        case 0:
            how->ctx.bit_bang_state = OW_BB_STATUS_BUSY;
            reset_pin(how->config.data_pin);
            set_timer_async(how->config.htim, 6);

            how->ctx.sub_state = 1;
            break;
        case 1:
            set_pin(how->config.data_pin);
            set_timer_async(how->config.htim, 9);

            how->ctx.sub_state = 2;
            break;
        case 2:
            const uint8_t sample = read_pin(how->config.data_pin);
            set_timer_async(how->config.htim, 55);

            how->current_byte |= (sample << how->bit_index);

            how->ctx.sub_state = 0;
            how->bit_index++;

            if (how->bit_index >= 8) {
                how->bit_index = 0;
                how->ctx.bit_bang_state = OW_BB_STATUS_OK;
            }
            break;
        default:
            how->ctx.state = OW_STATE_ERROR;
            how->ctx.last_error = OW_ERROR_INVALID_SUB_STATE;
            break;
    }
}

static void set_timer_async(TIM_HandleTypeDef *htim, const uint32_t us) {
    __HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE); // FIXME: Probably useless
    __HAL_TIM_SET_COUNTER(htim, 0);
    __HAL_TIM_SET_AUTORELOAD(htim, us);
    HAL_TIM_Base_Start_IT(htim);
}

static void set_pin(const GPIO_Pin_t *pin) {
    HAL_GPIO_WritePin(pin->port, pin->pin, GPIO_PIN_SET);
}

static void reset_pin(const GPIO_Pin_t *pin) {
    HAL_GPIO_WritePin(pin->port, pin->pin, GPIO_PIN_RESET);
}

static uint8_t read_pin(const GPIO_Pin_t *pin) {
    return HAL_GPIO_ReadPin(pin->port, pin->pin);
}
