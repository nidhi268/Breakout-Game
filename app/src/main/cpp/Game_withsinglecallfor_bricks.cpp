#include "Game.h"
#include "glm/gtc/matrix_transform.hpp"
#include "logging.h"

#include <sstream>

Game::Game(android_app *app) : camera(2.5f), renderer(app, camera) {
    spawn();
}

void Game::update(float dt) {
    RenderData data{camera, {.65f, .8f, 1.f}};

    GameState state{camera, dt};
    for (auto &object : objects) {
        if (object.on_update) {
            if(object.update_delay > 0.f){
                object.update_delay -= dt;
            }
            else{
                object.on_update(object, state);
            }
        }

        if (object.on_collide) {
            for (auto &other : objects) {
                if (&other == &object) {
                    continue;
                }

                auto difference = object.position - other.position;
                auto clamped = glm::clamp(difference, -other.size / 2.f, other.size / 2.f);
                auto closest = other.position + clamped;
                auto closest_difference = closest - object.position;

                if (glm::length(closest_difference) < object.collision_radius) {
                    object.on_collide(object, state, other, closest_difference);
                }
            }
        }

        auto T = glm::translate(glm::mat4{1.f}, glm::vec3(object.position, 0.f));
        auto S = glm::scale(glm::mat4{1.f}, glm::vec3(object.size, 0.f));

        data.draw_cmds.push_back(DrawCommand{T * S, object.texture, object.color});
    }
    if(state.should_restart){
        spawn();
    }

    auto end = std::remove_if(objects.begin(), objects.end(), [&](auto &obj){return state.to_delete.count(&obj) ==1;});
    objects.erase(end, objects.end());
    renderer.do_frame(data);
}

void Game::touch_event(glm::vec2 position, TouchEventType type) {
    position /= glm::vec2(renderer.get_size());
    position.y = 1.f - position.y;
    position *= 2.f;
    position -= 1.f;
    position = glm::inverse(camera.get_projection()) * glm::vec4(position, 0.f, 1.f);

    GameState state{camera};
    for (auto &object : objects) {
        if (!object.on_touch)
            continue;

        if (object.selected || (object.on_touch && position.x > object.position.x - object.size.x / 2.f
                                && position.x < object.position.x + object.size.x / 2.f
                                && position.y > object.position.y - object.size.y / 2.f
                                && position.y < object.position.y + object.size.y / 2.f)) {
            object.on_touch(object, state, position, type);
        }
    }

    if(state.should_restart){
        spawn();
    }

    auto end = std::remove_if(objects.begin(), objects.end(),
                              [&](auto &obj){return state.to_delete.count(&obj) ==1;});

    objects.erase(end, objects.end());
}

void Game::spawn() {
    objects = {
            GameObject{
                    .tag = "Player",
                    .position{},
                    .size = {1.f, .25f},
                    .color = {1.f, 1.f, 0.f, 1.f},
                    .on_update = [](GameObject &self, GameState &state) {
                        self.position.y = state.camera.bottom + 1.f;
                    },
                    .on_touch = [](GameObject &self, GameState &state, glm::vec2 pos, TouchEventType type) {
                        if (type == TouchEventType::Down) {
                            self.selected = true;
                            self.selection_offset = pos - self.position;
                        } else if (type == TouchEventType::Up) {
                            self.selected = false;
                        } else if (type == TouchEventType::Move && self.selected) {
                            self.position.x = std::clamp((pos - self.selection_offset).x,
                                                         state.camera.left + self.size.x / 2.f,
                                                         state.camera.right - self.size.x / 2.f);
                        }
                    },
            },
            GameObject{
                    .tag = "Ball",
                    .size = {.25f, .25f},
                    .color = {1.f, 1.f, 1.f, 1.f},
                    .direction = {0.f, 1.f},
                    .speed = 5.f,
                    .update_delay = 2.f,
                    .collision_radius = .125f,
                    .on_update = [](GameObject &self, GameState &state) {
                        self.position += self.direction * self.speed * state.dt;

                        if (self.position.y < state.camera.bottom + self.size.y / 2.f
                            && self.direction.y < 0.f) {
                            state.restart(); //when we hit the bottom line
                        }

                        if (self.position.y > state.camera.top - self.size.y / 2.f
                            && self.direction.y > 0.f) {
                            self.direction.y = -self.direction.y;
                        }

                        if (self.position.x < state.camera.left + self.size.x / 2.f
                            && self.direction.x < 0.f) {
                            self.direction.x = -self.direction.x;
                        }

                        if (self.position.x > state.camera.right - self.size.x / 2.f
                            && self.direction.x > 0.f) {
                            self.direction.x = -self.direction.x;
                        }
                    },

                    .on_collide = [](GameObject &self,
                                     GameState &state,
                                     GameObject &other,
                                     glm::vec2 difference) {
                        enum { UP, RIGHT, DOWN, LEFT };
                        glm::vec2 directions[]{{0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f}, {-1.0f, 0.0f}};

                        float max = 0.0f;
                        unsigned int direction;
                        for (unsigned int i = 0; i < 4; i++) {
                            float dot = glm::dot(glm::normalize(difference), directions[i]);

                            if (dot > max) {
                                max = dot;
                                direction = i;
                            }
                        }

                        if (direction == LEFT || direction == RIGHT) {
                            self.direction.x = -self.direction.x;
                            float penetration = self.collision_radius - std::abs(difference.x);
                            if (direction == LEFT) {
                                self.position.x += penetration;
                            } else {
                                self.position.x -= penetration;
                            }
                        } else {
                            self.direction.y = -self.direction.y;
                            float penetration = self.collision_radius - std::abs(difference.y);
                            if (direction == UP) {
                                self.position.y -= penetration;
                            } else {
                                self.position.y += penetration;
                            }
                        }

                        if (other.tag == "Player" && direction == DOWN) {
                            self.direction.x = (self.position.x - other.position.x) / (other.size.x / 2.f);
                            self.direction = glm::normalize(self.direction);
                        }

                        else if(other.tag == "Brick"){
                            state.destroy(other);
                        }
                    }
            },
    };
    constexpr uint32_t BRICKS_PER_ROW = 5;
    constexpr float BRICK_HEIGHT = .5f;
    float brick_width = camera.zoom * 2.f / BRICKS_PER_ROW;

    std::string map = R"(
1 1 1 1 1
1 2 0 2 1
3 0 4 0 3
0 5 5 5 0
1 6 6 6 1
)";

    std::array colors = {
            glm::vec4{1.f, .3f, .3f, 1.f},
            glm::vec4{.3f, 1.f, .3f, 1.f},
            glm::vec4{.3f, .3f, 1.f, 1.f},
            glm::vec4{.3f, 1.f, 1.f, 1.f},
            glm::vec4{1.f, .3f, 1.f, 1.f},
            glm::vec4{1.f, 1.f, 1.f, 1.f},
    };

    std::stringstream map_stream{map};
    std::vector<glm::vec2> brick_positions;
    std::vector<glm::vec4> brick_colors;

    std::string line;
    float y_position = camera.top - BRICK_HEIGHT / 2.f;
    while (std::getline(map_stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream line_stream{line};
        float x_position = camera.left + brick_width / 2.f;
        for (uint32_t i = 0; i < BRICKS_PER_ROW; i++) {
            int value;
            line_stream >> value;

//            if (value > 0) {
//                objects.push_back(GameObject{
//                        .tag = "Brick",
//                        .position = {x_position, y_position},
//                        .size = {brick_width * .95f, BRICK_HEIGHT * .95f},
//                        .color = colors[value <= colors.size() ? value - 1 : 0],
//                });
//            }
            if (value > 0) {
                brick_positions.emplace_back(x_position, y_position);
                brick_colors.emplace_back(colors[value <= colors.size() ? value - 1 : 0]);
            }

            x_position += brick_width;
        }

        y_position -= BRICK_HEIGHT;
    }

    renderer.set_bricks(brick_positions, brick_colors, brick_width, BRICK_HEIGHT);
}
