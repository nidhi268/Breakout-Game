#ifndef BREAKOUTGAME_GAME_H
#define BREAKOUTGAME_GAME_H

#pragma once

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "Renderer.h"
#include "Camera.h"
#include "unordered_set"
enum class TouchEventType {
    Down,
    Move,
    Up,
};

struct GameObject;

class GameState{
    friend class Game;

public:
    Camera &camera;
    float dt;

    void destroy(GameObject &obj){
        to_delete.insert(&obj);
    }

    void restart(){
        should_restart = true;
    }

private:
    explicit GameState(Camera &camera, float dt = 0.f) : camera(camera), dt(dt) {}

    std::unordered_set<GameObject*> to_delete{};
    bool should_restart = false;
};

struct GameObject {
    std::string tag{};
    glm::vec2 position{}, size{1.f};
    glm::vec4 color{1.f};
    Texture *texture{};

    bool selected{};
    glm::vec2 selection_offset{};

    glm::vec2 direction{};
    float speed{};
    float update_delay{};

    float collision_radius;

    std::function<void(GameObject &, GameState &) > on_update{};
    std::function<void(GameObject &, GameState &, GameObject &, glm::vec2)> on_collide{};
    std::function<void(GameObject &, GameState &, glm::vec2, TouchEventType)> on_touch{};
};

class Game {
public:
    explicit Game(android_app *app);

    void touch_event(glm::vec2 position, TouchEventType type);
    void update(float dt);

private:
    void spawn();
    Camera camera;
    Renderer renderer;

    std::vector<GameObject> objects;
};

#endif //BREAKOUTGAME_GAME_H
