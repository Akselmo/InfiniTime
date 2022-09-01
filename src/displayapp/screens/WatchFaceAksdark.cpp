#include <date/date.h>
#include <lvgl/lvgl.h>
#include "displayapp/screens/WatchFaceAksdark.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;



WatchFaceAksdark::WatchFaceAksdark(DisplayApp* app,
                                     Controllers::DateTime& dateTimeController,
                                     Controllers::Battery& batteryController,
                                     Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificatioManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::MotionController& motionController)
  : Screen(app),
    currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificatioManager {notificatioManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {
  settingsController.SetClockFace(3);

  //20 characters with default font size
  //240x240 res

  //BACKGROUND
  background = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_color(background, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x20, 0x20, 0x20));
  lv_obj_set_style_local_radius(background, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_size(background, 240, 240);
  lv_obj_align(background, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 0, 0);

  /*
label_prompt_1 = lv_label_create(lv_scr_act(), nullptr);
lv_label_set_text_static(label_prompt_1, "X--------XX--------X");
SetupLabel(label_prompt_1, LV_ALIGN_IN_TOP_LEFT, 0, 0, &plex_mono_20);

  label_prompt_2 = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_prompt_2, "X--------XX--------X");
  SetupLabel(label_prompt_2, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0, &plex_mono_20);
  */

  //BATTERY
  const int battery_x = 200;
  const int battery_y = -100;
  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(batteryValue, battery_x, battery_y, &plex_mono_20);
  chargeIcon = lv_label_create(lv_scr_act(), nullptr);
  batteryIconParent = lv_label_create(lv_scr_act(), nullptr);
  SetupBatteryIcon(batteryIconParent, battery_x-17, battery_y, 0x6ccc2d);

  //TIME AND DATE
  const int time_x = 20;
  const int time_y = -40;
  label_time = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(label_time, time_x, time_y, &plex_mono_42);
  label_date = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(label_date, time_x + 40, time_y + 40, &plex_mono_20);

  //STEP
  const int step_x = 40;
  const int step_y = 100;
  stepValue = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(stepValue, step_x, step_y, &plex_mono_20);
  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  SetupIcon(stepIcon, step_x - 27, step_y, 0xffbf40, Symbols::shoe);

  //HEARTBEAT
  const int heartbeat_x = step_x + 160;
  const int heartbeat_y = step_y;
  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(heartbeatValue, heartbeat_x, heartbeat_y, &plex_mono_20);
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  SetupIcon(heartbeatIcon, heartbeat_x - 25, heartbeat_y, 0xFF716A, Symbols::heartBeat);

  //NOTIF
  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  SetupIcon(notificationIcon, 110, 40, 0x21c7ca, NotificationIcon::GetIcon(true));

  //BLUETOOTH
  bluetoothIcon = lv_label_create(lv_scr_act(), nullptr);
  SetupIcon(bluetoothIcon, 20, -100, 0x3b96ff, Symbols::bluetooth);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceAksdark::~WatchFaceAksdark() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceAksdark::Refresh() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    lv_label_set_text_fmt(batteryValue, "#6ccc2d %d", batteryPercentRemaining.Get());
    UpdateBattery();
  }

  isCharging = batteryController.IsCharging();
  if (isCharging.IsUpdated()) {
    if (isCharging.Get()) {
      lv_obj_set_hidden(batteryIcon.GetObject(), true);
      lv_obj_set_hidden(chargeIcon, false);
    } else {
      lv_obj_set_hidden(batteryIcon.GetObject(), false);
      lv_obj_set_hidden(chargeIcon, true);
      UpdateBattery();
    }
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      lv_obj_set_style_local_text_color(bluetoothIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xfd2e59));
    } else {
      if (bleState.Get()) {
        lv_obj_set_style_local_text_color(bluetoothIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x3b96ff));
      } else {
        lv_obj_set_style_local_text_color(bluetoothIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xff1aba));
      }
    }
  }

  notificationState = notificatioManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    if (notificationState.Get())
    {
      lv_label_set_text_static(notificationIcon, Symbols::info);
      lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xfd2e59));

    }
    else
    {
      lv_label_set_text_static(notificationIcon, Symbols::check);
      lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x21c7ca));

    }

  }

  currentDateTime = dateTimeController.CurrentDateTime();

  if (currentDateTime.IsUpdated()) {
    auto newDateTime = currentDateTime.Get();

    auto dp = date::floor<date::days>(newDateTime);
    auto time = date::make_time(newDateTime - dp);
    auto yearMonthDay = date::year_month_day(dp);

    auto year = static_cast<int>(yearMonthDay.year());
    auto month = static_cast<Pinetime::Controllers::DateTime::Months>(static_cast<unsigned>(yearMonthDay.month()));
    auto day = static_cast<unsigned>(yearMonthDay.day());
    auto dayOfWeek = static_cast<Pinetime::Controllers::DateTime::Days>(date::weekday(yearMonthDay).iso_encoding());

    uint8_t hour = time.hours().count();
    uint8_t minute = time.minutes().count();
    uint8_t second = time.seconds().count();

    if (displayedHour != hour || displayedMinute != minute || displayedSecond != second) {
      displayedHour = hour;
      displayedMinute = minute;
      displayedSecond = second;
      lv_label_set_text_fmt(label_time, "#21c7ca %02d:%02d:%02d", hour, minute, second);
    }

    if ((year != currentYear) || (month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {
      lv_label_set_text_fmt(label_date, "#8394ff %02d-%02d-%04d#", char(day), char(month), short(year));

      currentYear = year;
      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "#FF716A %d#", heartbeat.Get());
      lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
    } else {
      lv_label_set_text_static(heartbeatValue, "#FF716A -#");
      lv_label_set_text(heartbeatIcon, Symbols::heartBeat);
    }
  }

  stepCount = motionController.NbSteps();
  motionSensorOk = motionController.IsSensorOk();
  if (stepCount.IsUpdated() || motionSensorOk.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "#ffbf40 %lu#", stepCount.Get());
  }
}

void WatchFaceAksdark::SetupLabel(lv_obj_t* label, int x_ofs, int y_ofs, _lv_font_struct* font)
{
  lv_label_set_recolor(label, true);
  lv_obj_align(label, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, x_ofs, y_ofs);
  lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font);
}

void WatchFaceAksdark::SetupIcon(lv_obj_t* label, int x_ofs, int y_ofs, uint32_t hexColor, const char* icon)
{
  lv_obj_align(label, nullptr, LV_ALIGN_IN_LEFT_MID, x_ofs, y_ofs);
  lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(hexColor));
  lv_label_set_text_static(label, icon);
}

void WatchFaceAksdark::SetupBatteryIcon(lv_obj_t* batteryIconParent, int x_ofs, int y_ofs, uint32_t hexColor)
{
  lv_obj_align(batteryIconParent, nullptr, LV_ALIGN_IN_LEFT_MID, x_ofs, y_ofs);
  lv_label_set_text_static(batteryIconParent, " ");
  batteryIcon.Create(batteryIconParent);
  batteryIcon.SetColor(lv_color_hex(hexColor));
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_LEFT_MID, 0, 0);
  SetupIcon(chargeIcon, x_ofs, y_ofs, hexColor, Symbols::plug);
}

void WatchFaceAksdark::UpdateBattery()
{
  auto batteryPercent = batteryPercentRemaining.Get();
  batteryIcon.SetBatteryPercentage(batteryPercent);
}