// Startet den ECHTEN WebController samt allen Szenen und bedient ihn ueber stdin/stdout:
//   Eingabe : METHODE<TAB>URI<TAB>urlencoded-body
//   Ausgabe : HTTP-Code<TAB>Content-Encoding<TAB>Body
#include <iostream>
#include <sstream>
#include "app/SceneManager.h"
#include "display/DisplayDriver.h"
#include "network/NetworkService.h"
#include "network/WebController.h"
#include "scenes/ClockScene.h"
#include "scenes/DigitalRainScene.h"
#include "scenes/FireScene.h"
#include "scenes/MandelbrotScene.h"
#include "scenes/AutumnLakeScene.h"
#include "scenes/DogFetchScene.h"
#include "scenes/DriveScene.h"
#include "scenes/MoonIslandScene.h"
#include "scenes/ManualTextScene.h"
#include "scenes/PlasmaScene.h"
#include "scenes/StarfieldScene.h"
#include "scenes/WarpScene.h"
#include "scenes/WorldClockScene.h"
#include "services/TimeService.h"
#include "storage/SettingsStore.h"

static std::string urldecode(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') o += ' ';
        else if (s[i] == '%' && i + 2 < s.size()) { o += (char)std::stoi(s.substr(i + 1, 2), nullptr, 16); i += 2; }
        else o += s[i];
    }
    return o;
}

int main() {
    DisplayDriver display; display.begin(DisplayTuning());
    TimeService timeService; SettingsStore settings; settings.begin();
    NetworkService network(settings); SceneManager scenes;
    ManualTextScene text(display); ClockScene clock(display, timeService); WorldClockScene world(display, timeService);
    FireScene fire(display); DigitalRainScene rain(display);
    StarfieldScene stars(display); WarpScene warp(display); PlasmaScene plasma(display);
    MandelbrotScene mandel(display); AutumnLakeScene autumn(display); MoonIslandScene island(display); DogFetchScene dog(display); DriveScene drive(display);
    for (IScene* s : std::initializer_list<IScene*>{&text, &clock, &world, &fire, &rain, &stars, &warp, &plasma, &mandel, &autumn, &island, &dog, &drive}) scenes.add(*s);
    scenes.activate("fire");
    WebController web(scenes, text, display, settings, network, timeService);
    web.begin();
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string method, uri, body;
        std::getline(ss, method, '\t'); std::getline(ss, uri, '\t'); std::getline(ss, body);
        wt::args.clear(); wt::curUri = uri; wt::outBody.clear(); wt::outEnc.clear(); wt::outCode = 0;
        std::istringstream bs(body); std::string kv;
        while (std::getline(bs, kv, '&')) {
            const size_t eq = kv.find('=');
            if (eq == std::string::npos) continue;
            wt::args[urldecode(kv.substr(0, eq))] = urldecode(kv.substr(eq + 1));
        }
        auto it = wt::routes.find(method + " " + uri);
        if (it != wt::routes.end()) it->second(); else wt::notFound();
        scenes.update();
        std::cout << wt::outCode << '\t' << wt::outEnc << '\t' << wt::outBody << std::endl;
    }
}
