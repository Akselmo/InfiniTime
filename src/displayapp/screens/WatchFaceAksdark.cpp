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

  //TEXT LABELS
  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(batteryValue, LV_ALIGN_IN_LEFT_MID, 0, -20, &plex_mono_20);

  connectState = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(connectState, LV_ALIGN_IN_LEFT_MID, 0, 40, &plex_mono_20);

  /*
  label_prompt_1 = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_prompt_1, "X--------XX--------X");
  SetupLabel(label_prompt_1, LV_ALIGN_IN_TOP_LEFT, 0, 0, &plex_mono_20);

  label_prompt_2 = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_prompt_2, "X--------XX--------X");
  SetupLabel(label_prompt_2, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0, &plex_mono_20);
  */

  label_time = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(label_time, LV_ALIGN_IN_LEFT_MID, 20, -60, &plex_mono_42);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(label_date, LV_ALIGN_IN_LEFT_MID, 60, -20, &plex_mono_20);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(heartbeatValue, LV_ALIGN_IN_LEFT_MID, 0, 20, &plex_mono_20);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  SetupLabel(stepValue, LV_ALIGN_IN_LEFT_MID, 0, 0, &plex_mono_20);


  // ICONS
  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_LEFT_MID, 0, -100);


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
    lv_label_set_text_fmt(batteryValue, "#6ccc2d %d%%", batteryPercentRemaining.Get());
    if (batteryController.IsPowerPresent()) {
      lv_label_ins_text(batteryValue, LV_LABEL_POS_LAST, " Charging");
    }
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      lv_label_set_text_static(connectState, "#ff1aba Disabled#");
    } else {
      if (bleState.Get()) {
        lv_label_set_text_static(connectState, "#3b96ff Connected#");
      } else {
        lv_label_set_text_static(connectState, "#ff1aba Disconnected#");
      }
    }
  }

  notificationState = notificatioManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
    lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x21, 0xc7, 0xca));
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
      lv_label_set_text_fmt(heartbeatValue, "HB: #fd2e59 %d bpm#", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "");
    }
  }

  stepCount = motionController.NbSteps();
  motionSensorOk = motionController.IsSensorOk();
  if (stepCount.IsUpdated() || motionSensorOk.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "#ffbf40 %lu steps#", stepCount.Get());
  }
}

void WatchFaceAksdark::SetupLabel(lv_obj_t* label, int align, int x_ofs, int y_ofs, _lv_font_struct* font)
{
  lv_label_set_recolor(label, true);
  lv_obj_align(label, lv_scr_act(), align, x_ofs, y_ofs);
  lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font);
}