#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <displayapp/Controllers.h>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/SimpleWeatherService.h"
#include "utility/DirtyValue.h"
#include "displayapp/screens/BatteryIcon.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceTerminal : public Screen {
      public:
        WatchFaceTerminal(Controllers::DateTime& dateTimeController,
                          const Controllers::Battery& batteryController,
                          const Controllers::Ble& bleController,
                          Controllers::NotificationManager& notificationManager,
                          Controllers::Settings& settingsController,
                          Controllers::SimpleWeatherService& weatherService);
        ~WatchFaceTerminal() override;

        void Refresh() override;

      private:
        Utility::DirtyValue<int> batteryPercentRemaining {};
        Utility::DirtyValue<bool> powerPresent {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<bool> bleRadioEnabled {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>> currentDateTime {};
        Utility::DirtyValue<bool> notificationState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};

        lv_obj_t* connectState;
        lv_obj_t* label_time;
        lv_obj_t* label_date;
        lv_obj_t* notificationIcon;
        lv_obj_t* labelPrompt1;
        lv_obj_t* labelTime;
        lv_obj_t* labelDate;
        lv_obj_t* batteryValue;
        lv_obj_t* stepValue;
        lv_obj_t* heartbeatValue;
        lv_obj_t* weather;
        lv_obj_t* weatherIcon;

        lv_obj_t* topleft_h;
        lv_obj_t* topleft_w;

        lv_obj_t* bottomright_h;
        lv_obj_t* bottomright_w;

        lv_color_t defaultCornerColor;
        lv_color_t activityCornerColor;
        lv_color_t warningCornerColor;

        BatteryIcon batteryIcon;
        int batteryPercent = 0;

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::SimpleWeatherService& weatherService;

        lv_task_t* taskRefresh;

        void recolorCorners(const lv_color_t& newColor);
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Terminal> {
      static constexpr WatchFace watchFace = WatchFace::Terminal;
      static constexpr const char* name = "Aks";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceTerminal(controllers.dateTimeController,
                                              controllers.batteryController,
                                              controllers.bleController,
                                              controllers.notificationManager,
                                              controllers.settingsController,
                                              *controllers.weatherController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
