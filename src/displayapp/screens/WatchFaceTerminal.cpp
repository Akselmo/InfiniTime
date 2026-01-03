#include <lvgl/lvgl.h>
#include "displayapp/screens/WatchFaceTerminal.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/settings/Settings.h"
#include "components/ble/SimpleWeatherService.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "displayapp/InfiniTimeTheme.h"
#include "lvgl/src/lv_core/lv_disp.h"

using namespace Pinetime::Applications::Screens;

WatchFaceTerminal::WatchFaceTerminal(Controllers::DateTime& dateTimeController,
                                     const Controllers::Battery& batteryController,
                                     const Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificationManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::SimpleWeatherService& weatherService)
  : currentDateTime {{}},
    defaultCornerColor {lv_color_hex(0x33ffff)},
    activityCornerColor {lv_color_hex(0x00ec00)},
    warningCornerColor {lv_color_hex(0xff3344)},
    batteryIcon(false),
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    weatherService {weatherService} {

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_time, true);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, -10);
  lv_label_set_align(label_time, LV_LABEL_ALIGN_CENTER);
  lv_obj_set_auto_realign(label_time, true);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_date, true);
  lv_obj_align(label_date, label_time, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
  lv_label_set_align(label_date, LV_LABEL_ALIGN_CENTER);
  lv_obj_set_auto_realign(label_date, true);

  weather = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(weather, true);
  lv_obj_align(weather, label_date, LV_ALIGN_OUT_BOTTOM_MID, 14, 12);

  weatherIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(weatherIcon, true);
  lv_obj_align(weatherIcon, weather, LV_ALIGN_OUT_LEFT_MID, -4, 0);
  lv_obj_set_style_local_text_color(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x838fff));
  lv_obj_set_style_local_text_font(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &fontawesome_weathericons);
  lv_label_set_text(weatherIcon, "");
  lv_obj_set_auto_realign(weatherIcon, true);

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -4, 4);

  connectState = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(connectState, true);
  lv_obj_set_style_local_text_color(connectState, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xff3344));
  lv_obj_align(connectState, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 4, -4);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(notificationIcon, label_time, LV_ALIGN_OUT_TOP_MID, 0, -16);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, activityCornerColor);

  // Corners
  topleft_h = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_radius(topleft_h, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_size(topleft_h, 4, 16);
  lv_obj_align(topleft_h, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 2, 2);
  topleft_w = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_radius(topleft_w, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_size(topleft_w, 16, 4);
  lv_obj_align(topleft_w, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 2, 2);

  bottomright_h = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_radius(bottomright_h, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_size(bottomright_h, 4, 16);
  lv_obj_align(bottomright_h, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -2, -2);
  bottomright_w = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_radius(bottomright_w, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_size(bottomright_w, 16, 4);
  lv_obj_align(bottomright_w, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -2, -2);

  recolorCorners(defaultCornerColor);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceTerminal::~WatchFaceTerminal() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceTerminal::Refresh() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    batteryPercent = batteryPercentRemaining.Get();
    batteryIcon.SetBatteryPercentage(batteryPercent);
    if (batteryPercent > 10) {
      batteryIcon.SetColor(lv_color_hex(0x282828));
    } else {
      batteryIcon.SetColor(lv_color_hex(0xff3344));
    }
    if (batteryController.IsPowerPresent()) {
      batteryIcon.SetColor(lv_color_hex(0x00ec00));
    }
    lv_obj_realign(batteryIcon.GetObject());
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    if (notificationState.Get()) {
      lv_label_set_text_static(notificationIcon, Symbols::bell);
    } else {
      lv_label_set_text_static(notificationIcon, "");
    }
    lv_obj_realign(notificationIcon);
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      lv_label_set_text(connectState, "BL OFF");
      lv_obj_set_style_local_bg_color(connectState, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x282828));
    } else {
      if (!bleState.Get()) {
        lv_label_set_text(connectState, Symbols::bluetooth);
        lv_obj_set_style_local_bg_color(connectState, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xff3344));
      } else {
        lv_label_set_text(connectState, "");
      }
    }
    lv_obj_realign(connectState);
  }

  if (notificationState.Get()) {
    recolorCorners(activityCornerColor);
  } else if (!bleRadioEnabled.Get() || !bleState.Get() || batteryPercent <= 10) {
    recolorCorners(warningCornerColor);
  } else {
    recolorCorners(defaultCornerColor);
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();
    lv_label_set_text_fmt(label_time, "#33ffff  %02d:%02d", hour, minute);

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      Controllers::DateTime::Months month = dateTimeController.Month();
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(label_date, "#ffffff %02d/%02d/%04d#", char(day), char(month), short(year));
    }
  }
  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    auto optCurrentWeather = currentWeather.Get();
    if (optCurrentWeather) {
      int16_t temp = optCurrentWeather->temperature.Celsius();
      char tempUnit = 'C';
      if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
        temp = optCurrentWeather->temperature.Fahrenheit();
        tempUnit = 'F';
      }
      lv_label_set_text_fmt(weather, "#838fff %d°%c#", temp, tempUnit);
      lv_label_set_text(weatherIcon, Symbols::GetSymbol(optCurrentWeather->iconId, weatherService.IsNight()));

    } else {
      lv_label_set_text(weather, "");
      lv_label_set_text(weatherIcon, "");
    }
    lv_obj_realign(weather);
    lv_obj_realign(weatherIcon);
  }
}

void WatchFaceTerminal::recolorCorners(const lv_color_t& newColor) {
  lv_obj_set_style_local_bg_color(topleft_h, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, newColor);
  lv_obj_set_style_local_bg_color(topleft_w, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, newColor);
  lv_obj_set_style_local_bg_color(bottomright_h, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, newColor);
  lv_obj_set_style_local_bg_color(bottomright_w, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, newColor);
}
