#include "SceneManager.h"
#include <string.h>

bool SceneManager::add(IScene& scene) {
    if (count_ >= MAX_SCENES || hasScene(scene.id())) return false;
    scenes_[count_++] = &scene;
    return true;
}

IScene* SceneManager::find(const String& sceneId) const {
    for (uint8_t i = 0; i < count_; ++i)
        if (sceneId == scenes_[i]->id()) return scenes_[i];
    return nullptr;
}

bool SceneManager::activate(const String& sceneId) {
    IScene* next = find(sceneId);
    if (next == nullptr) return false;
    sceneStartMs_ = millis();
    if (active_ == next) return true;
    if (active_ != nullptr) active_->end();
    active_ = next;
    active_->begin();
    lastFrameMs_ = 0;
    Serial.printf("[scene] active: %s (%s)\n", active_->id(), active_->title());
    return true;
}

void SceneManager::update() {
    if (active_ == nullptr) return;
    const uint32_t now = millis();
    if (demo_.enabled && now - sceneStartMs_ >= (uint32_t)demo_.intervalSec * 1000UL) advanceDemo();
    if (lastFrameMs_ != 0 && now - lastFrameMs_ < active_->frameIntervalMs()) return;
    lastFrameMs_ = now ? now : 1;
    active_->update();
}

bool SceneManager::demoIncludes(const char* id) const {
    if (strcmp(id, "sysinfo") == 0) return false;   // diagnostics screen is never part of the demo
    if (demo_.ids.length() == 0) return true;
    const String needle = String(",") + id + ",";
    return (String(",") + demo_.ids + ",").indexOf(needle) >= 0;
}

void SceneManager::setDemo(const DemoConfig& cfg) {
    demo_ = cfg;
    sceneStartMs_ = millis();
}

void SceneManager::advanceDemo() {
    sceneStartMs_ = millis();
    int cur = -1;
    for (uint8_t i = 0; i < count_; ++i) if (scenes_[i] == active_) cur = i;
    for (uint8_t step = 1; step <= count_; ++step) {
        IScene* cand = scenes_[(cur + step) % count_];
        if (cand == active_) continue;
        if (!demoIncludes(cand->id()) || !cand->available()) continue;
        activate(cand->id());
        return;
    }
}

const char* SceneManager::activeId() const {
    return active_ == nullptr ? "none" : active_->id();
}

const char* SceneManager::activeTitle() const {
    return active_ == nullptr ? "Keine Szene" : active_->title();
}
