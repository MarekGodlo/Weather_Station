/**
 * @file ui_ctrl.h
 *
 * Created on 8/21/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Wraps the EPD module to manage the UI shown on the e-paper display.
 */

#ifndef WEATHER_STATION_UI_CTRL_H
#define WEATHER_STATION_UI_CTRL_H
#include "../EPD/epd.h"

/**
 * @brief Represents the status code returned by UI_Ctrl functions.
 */
typedef enum {
    UI_CTRL_OK = 0,     /**< Operation finished successfully. */
    UI_CTRL_INIT_ERR,   /**< Failed initialization procedure. */
    UI_CTRL_UPDATE_ERR, /**< Failed to update the display content. */
} UI_Ctrl_Status_t;

/**
 * @brief Represents the UI_Ctrl handle.
 */
typedef struct {
    EPD_Handle_t *hepd; /**< Pointer to the EPD handle structure. */
} UI_Ctrl_Handle_t;

/**
 * @brief Initializes the e-paper display.
 * The contents of @p config are copied into the internal EPD handle.
 *
 * @param hui    Pointer to the UI_Ctrl handle structure.
 * @param config Pointer to the EPD config structure.
 *
 * @return Status of the operation.
 */
UI_Ctrl_Status_t UI_Ctrl_Init(const UI_Ctrl_Handle_t *hui, const EPD_Config_t *config);


/**
 * @brief Renders all static elements (sensor values) into the GFX buffer.
 *
 * @param hui Pointer to the UI_Ctrl handle structure.
 */
void UI_Ctrl_DisplayStaticElements(UI_Ctrl_Handle_t *hui);

/**
 * @brief Renders all dynamic elements into the GFX buffer (It is values).
 *
 * @param hui         Pointer to the UI_Ctrl handle structure.
 * @param temp_in     Internal temperature value, in degrees Celsius.
 * @param humidity_in Internal humidity value, in percent.
 * @param pressure_in Internal pressure value, in Pa.
 * @param temp_out    External temperature value, in degrees Celsius.
 */
void UI_Ctrl_DisplayValues(UI_Ctrl_Handle_t *hui, float temp_in, float humidity_in, uint32_t pressure_in, float temp_out);

/**
 * @brief Updates the e-paper display via full refresh.
 *
 * @param hui Pointer to the UI_Ctrl handle structure.
 *
 * @return Status of the operation.
 */
UI_Ctrl_Status_t UI_Ctrl_Update(const UI_Ctrl_Handle_t *hui);

/**
 * @brief Updates the e-paper display via partial refresh.
 *
 * @param hui Pointer to the UI_Ctrl handle structure.
 *
 * @return Status of the operation.
 */
UI_Ctrl_Status_t UI_Ctrl_UpdatePartial(const UI_Ctrl_Handle_t *hui);
#endif //WEATHER_STATION_UI_CTRL_H
