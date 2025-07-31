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
        android_app_set_motion_event_filter(app, nullptr);

        do {
            bool done = false;
            do {
                android_poll_source *poll_source;
                int events{};
                // gets all events from app
                int result = ALooper_pollOnce(0, nullptr, &events, (void **) &poll_source);

                switch (result){
                    case ALOOPER_POLL_WAKE:
                    case ALOOPER_POLL_TIMEOUT:
                        done = true;
                        break;
                    case ALOOPER_POLL_ERROR:
                        LOGE("ALooper_pollOnce returned an error");
                        break;
                    case ALOOPER_POLL_CALLBACK:
                        break;
                    default:
                        if(poll_source) poll_source->process(app, poll_source);
                }
            } while(!done);

            if (!app->userData) continue;
            auto *game = (Game *) app->userData;

            android_input_buffer *input_buffer = android_app_swap_input_buffers(app);
            if (input_buffer){
                for (size_t i = 0; i < input_buffer->motionEventsCount; i++){
                    GameActivityMotionEvent *motion_event = &input_buffer->motionEvents[i];
                    if (motion_event->pointerCount > 0){
                        int32_t masked_action = motion_event->action & AMOTION_EVENT_ACTION_MASK;
                        float x = GameActivityPointerAxes_getX(&motion_event->pointers[0]);
                        float y = GameActivityPointerAxes_getY(&motion_event->pointers[0]);

                        TouchEventType type;
                        bool valid = true;
                        switch (masked_action){
                            case AMOTION_EVENT_ACTION_DOWN:
                                type = TouchEventType::Down;
                                break;
                            case AMOTION_EVENT_ACTION_UP:
                                type = TouchEventType::Up;
                                break;
                            case AMOTION_EVENT_ACTION_MOVE:
                                type = TouchEventType::Move;
                                break;
                            default:
                                valid = false;
                                break;
                        }
                        if(valid) game -> touch_event({x, y}, type);
                    }
                }

                android_app_clear_motion_events(input_buffer);
            }

            game->update();
        } while (!app->destroyRequested);
    }
}