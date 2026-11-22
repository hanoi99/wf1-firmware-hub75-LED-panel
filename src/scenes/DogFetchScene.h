#pragma once

#include "BaseScene.h"

// A small blond dog with pointed ears on a meadow: waits, the ball is thrown, the dog runs out,
// picks it up, brings it back and drops it - in an endless loop. Fully procedural.
class DogFetchScene : public BaseScene {
public:
    explicit DogFetchScene(DisplayDriver& display);
    const char* id() const override { return "dog"; }
    const char* title() const override { return "Hund mit Ball"; }
    uint16_t frameIntervalMs() const override { return 50; }
    void begin() override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_CLOUDS, O_SPEED };
    enum DogPhase : uint8_t { SIT, RUN_OUT, PICK, RUN_BACK, DROP };
    enum BallPhase : uint8_t { NONE, FLY, REST, CARRIED, GROUND_AT_DOG };
    void step();
    void drawDog(int x, int y, int dir, DogPhase pose, int frame);
    void drawBall(int x, int y);

    DogPhase dog_ = SIT;
    BallPhase ball_ = NONE;
    int t_ = 0;               // frames in the current dog phase
    int bt_ = 0;              // frames of the ball flight
    int dogX_ = 0;            // x * 16
    int targetX_ = 44;        // landing spot
    int ballX_ = 0, ballY_ = 0;
    int bounce_ = 0;
    int tick_ = 0;
};
