#include "Game.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace runner;
static int checks=0;
static void check(bool condition,const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
static bool near(double a,double b,double eps=1e-5) { return std::abs(a-b)<eps; }
int main() {
    try {
        Game game;
        const auto first=game.obstacles()[0];
        game.update(10);
        check(game.state()==RunState::Ready && game.distance()==0,"Ready must not advance");
        game.startOrResume(); game.update(1.0);
        check(near(game.distance(),8),"Score must integrate the shared 8 m/s speed * dt");
        check(near(game.obstacles()[0].z,first.z+8),"Obstacles must approach along +Z");
        game.togglePause(); game.update(30);
        check(near(game.distance(),8),"Pause must freeze score");
        check(near(game.obstacles()[0].z,first.z+8),"Pause must freeze transforms");
        game.startOrResume(); game.update(3);
        check(game.recycled()>0,"Passing obstacles must recycle");
        const auto count=game.obstacles().size();
        game.update(3600); // many wraps in one step
        check(game.obstacles().size()==count,"Pool size must remain constant");
        for (const auto& o:game.obstacles())
            check(o.z<=Game::RecycleZ && o.z>=Game::RecycleZ-game.loopLength()-0.001,"Wrapped Z is bounded");
        check(game.groundPhase()>=0 && game.groundPhase()<Game::GroundPeriod,"Ground phase is bounded");
        game.reset();
        check(game.state()==RunState::Ready && game.distance()==0 && game.recycled()==0,"Reset must clear state");
        check(near(game.obstacles()[0].z,first.z),"Reset restores deterministic pattern");

        Game slow,fast;
        slow.startOrResume(); fast.startOrResume();
        for(int i=0;i<30*75;++i) slow.update(1.0/30.0);
        for(int i=0;i<144*75;++i) fast.update(1.0/144.0);
        check(near(slow.distance(),fast.distance(),1e-7),"Distance must not depend on frame rate");
        for(std::size_t i=0;i<slow.obstacles().size();++i)
            check(near(slow.obstacles()[i].z,fast.obstacles()[i].z,0.0001),"Movement must not depend on frame rate");
        Game longRun(3); longRun.startOrResume();
        for(int i=0;i<10000;++i) longRun.update(3600);
        check(near(longRun.distance(),288000000.0),"Long-distance score must retain precision");
        check(longRun.groundPhase()<HeightField::period,"Long-running texture phase remains bounded");
        for (const auto& o:longRun.obstacles())
            check(std::isfinite(o.z) && o.z<=12 && o.z>=12-longRun.loopLength(),"Long-run render coordinates remain bounded");
        for(double dt:{-1.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
            bool threw=false;
            try { game.update(dt); } catch(const std::invalid_argument&) { threw=true; }
            check(threw,"Invalid time must be rejected");
        }
        bool threw=false;
        try { Game invalid(0); } catch(const std::invalid_argument&) { threw=true; }
        check(threw,"Empty obstacle configuration must be rejected");
        std::cout<<"PASS: "<<checks<<" assertions; ready, motion, score, pause, recycle, reset, 75-second FPS independence, long-run precision, invalid input\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
