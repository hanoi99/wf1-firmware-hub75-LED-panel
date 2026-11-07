#pragma once

#include "scenes/IScene.h"

struct DemoConfig {
    bool enabled = false;
    uint16_t intervalSec = 20;
    String ids;  // comma separated; empty = all scenes
};

// Owns the scene registry, throttles update() per scene and runs the optional
// demo playlist. Exactly one scene is active at a time.
class SceneManager {
public:
    static constexpr uint8_t MAX_SCENES = 16;

    bool add(IScene& scene);
    bool activate(const String& sceneId);
    void update();

    const char* activeId() const;
    const char* activeTitle() const;
    IScene* active() const { return active_; }
    IScene* find(const String& sceneId) const;
    IScene* at(uint8_t index) const { return index < count_ ? scenes_[index] : nullptr; }
    uint8_t count() const { return count_; }
    bool hasScene(const String& sceneId) const { return find(sceneId) != nullptr; }

    void setDemo(const DemoConfig& cfg);
    const DemoConfig& demo() const { return demo_; }
    bool demoIncludes(const char* id) const;

private:
    void advanceDemo();
    IScene* scenes_[MAX_SCENES]{};
    uint8_t count_ = 0;
    IScene* active_ = nullptr;
    uint32_t lastFrameMs_ = 0;
    uint32_t sceneStartMs_ = 0;
    DemoConfig demo_;
};
