#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include <android/log.h>
#include "logging.h"
#include "Renderer.h"
#include "Game.h"

extern "C" {
#include <game-activity/native_app_glue/android_native_app_glue.c>
    void on_app_cmd(android_app *app, int32_t cmd){
        switch (cmd){
            case APP_CMD_INIT_WINDOW:
                LOGI("Initializing the window...");
                // sets renderer up at correct time
                app->userData = new Game(app);
                break;
            case APP_CMD_TERM_WINDOW: {
                LOGI("Terminating the window...");
                auto *game = (Game *) app->userData;
                delete game;
            }
                break;
            default:
                break;
        }
    }
    void android_main(android_app *app){
        app->onAppCmd = on_app_cmd;

        do {
            android_poll_source *poll_source;
            int events{};
            // gets all events from app
            int result = ALooper_pollOnce(0, nullptr, &events, (void **) &poll_source);
            if(result >= 0 && poll_source) poll_source->process(app, poll_source);

            if (!app->userData) continue;

            auto *game = (Game *) app->userData;
            game->update();
        } while (!app->destroyRequested);
    }
}