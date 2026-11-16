#include "Game.h"
#include "Player.h"
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
static double timeToTravel(double metres) {
    const double v=Game::BaseSpeed,a=Game::Acceleration;
    return (-v+std::sqrt(v*v+2.0*a*metres))/a;
}
int main() {
    try {
        Game game;
        const auto first=game.obstacles()[0];
        const auto last=game.obstacles().back();
        game.update(10);
        check(game.state()==RunState::Ready && game.distance()==0,"Ready must not advance");
        check(game.speed()==0 && game.currentSpeed()==Game::BaseSpeed,"Ready world must stand still at the base speed");
        game.startOrResume();
        check(game.speed()==Game::BaseSpeed,"Run must start at 8 m/s");
        game.update(1.0);
        check(near(game.distance(),8.1),"Score must integrate the accelerating speed");
        check(near(game.speed(),8.2),"Speed must grow by 0.2 m/s per second");
        check(near(game.obstacles()[0].z,first.z+game.distance(),1e-4),"Obstacles must approach by the score travel");
        check(near(game.groundPhase(),game.distance()),"Ground phase must use the same travel");
        const double paused=game.distance();
        game.togglePause(); game.update(30);
        check(game.state()==RunState::Paused && game.speed()==0,"Pause must stop the world speed");
        check(near(game.currentSpeed(),8.2),"Pause must keep the run speed");
        check(near(game.distance(),paused),"Pause must freeze score");
        check(near(game.obstacles()[0].z,first.z+paused,1e-4),"Pause must freeze transforms");
        game.startOrResume(); game.update(3);
        check(game.recycled()>0,"Passing obstacles must recycle");
        check(near(game.obstacles().back().z,last.z+game.distance(),1e-3),"Unwrapped obstacles must move by the score travel");
        check(near(game.groundPhase(),std::fmod(game.distance(),Game::GroundPeriod),1e-6),"Phase and score must not drift");
        const auto count=game.obstacles().size();
        game.update(3600);
        check(game.obstacles().size()==count,"Pool size must remain constant");
        for (const auto& o:game.obstacles())
            check(o.z<=Game::RecycleZ && o.z>=Game::RecycleZ-game.loopLength()-0.001,"Wrapped Z is bounded");
        check(game.groundPhase()>=0 && game.groundPhase()<Game::GroundPeriod,"Ground phase is bounded");
        check(game.speed()==Game::MaxSpeed,"Speed must stop at 22 m/s");
        game.reset();
        check(game.state()==RunState::Ready && game.distance()==0 && game.recycled()==0 && game.elapsed()==0,"Reset must clear state");
        check(game.currentSpeed()==Game::BaseSpeed,"Reset must restore the base speed");
        check(near(game.obstacles()[0].z,first.z),"Reset restores deterministic pattern");

        check(Game::speedAt(0)==8 && near(Game::speedAt(35),15) && Game::speedAt(70)==22 && Game::speedAt(1e6)==22,
            "Speed ramps from 8 to 22 m/s");
        double previous=Game::speedAt(0);
        bool smooth=true, integrated=true;
        for(int i=1;i<=12000;++i) {
            const double t=i*0.01, v=Game::speedAt(t);
            smooth=smooth && v>=previous && v-previous<=Game::Acceleration*0.01+1e-12 && v<=Game::MaxSpeed;
            integrated=integrated && near(Game::distanceAt(t)-Game::distanceAt(t-0.01),0.005*(v+previous),1e-9);
            previous=v;
        }
        check(smooth,"Speed must grow smoothly and stay capped");
        check(integrated,"Distance must integrate the speed curve");

        Game slow,fast,single;
        slow.startOrResume(); fast.startOrResume(); single.startOrResume();
        for(int i=0;i<30*75;++i) slow.update(1.0/30.0);
        for(int i=0;i<144*75;++i) fast.update(1.0/144.0);
        single.update(75);
        check(near(slow.distance(),fast.distance(),1e-7) && near(slow.distance(),single.distance(),1e-7),"Distance must not depend on frame rate");
        check(near(slow.currentSpeed(),fast.currentSpeed(),1e-9) && fast.currentSpeed()==Game::MaxSpeed,"Speed must not depend on frame rate");
        check(near(slow.groundPhase(),fast.groundPhase(),1e-6),"Ground phase must not depend on frame rate");
        for(std::size_t i=0;i<slow.obstacles().size();++i) {
            check(near(slow.obstacles()[i].z,fast.obstacles()[i].z,0.0001),"Movement must not depend on frame rate");
            check(near(slow.obstacles()[i].z,single.obstacles()[i].z,0.0001),"Movement must not depend on step size");
        }
        Game longRun(3); longRun.startOrResume();
        for(int i=0;i<10000;++i) longRun.update(3600);
        check(near(longRun.distance(),791999510.0,1e-3),"Long-distance score must retain precision");
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

        const GroundHeight flat=[](float,float) { return 0.0f; };
        const GroundHeight raised=[](float,float) { return 2.0f; };
        Game hit; hit.startOrResume();
        const auto lead=hit.obstacles()[0];
        check(lead.x==0 && near(lead.height,1.2),"Lead obstacle must sit in the centre lane");
        hit.update(timeToTravel(-lead.z));
        check(near(hit.obstacles()[0].z,0,1e-4),"Lead obstacle must reach the player plane");
        check(hit.collides(0,0,flat),"Head-on overlap must collide");
        check(hit.collides(0,1.19f,flat),"Feet below the obstacle top must collide");
        check(!hit.collides(0,1.21f,flat),"Feet above the obstacle top must not collide");
        check(!hit.collides(1.5f,0,flat),"Gap between lanes must not collide");
        check(!hit.collides(lead.width*0.5f+Game::PlayerWidth*0.5f+0.01f,0,flat),"Side clearance must not collide");
        check(hit.collides(lead.width*0.5f+Game::PlayerWidth*0.5f-0.01f,0,flat),"Side overlap must collide");
        check(!hit.collides(0,0,raised),"Obstacle base must follow the ground height");
        check(hit.collides(0,2.5f,raised),"Raised obstacle must collide with raised player");
        check(Game::onTerrain({0,0,1,1,1}) && !Game::onTerrain({0,-170,1,1,1}) && !Game::onTerrain({0,15.5f,1,1,1.35f}),
            "Only obstacles drawn on the terrain may collide");
        Game far; far.startOrResume();
        check(!far.collides(0,0,flat),"Distant obstacles must not collide");
        check(hit.checkCollision(0,0,flat) && hit.state()==RunState::GameOver,"Collision must end the run");
        const double crashDistance=hit.distance(); const float crashZ=hit.obstacles()[0].z;
        const double crashPhase=hit.groundPhase(), crashSpeed=hit.currentSpeed();
        hit.update(5); hit.togglePause(); hit.startOrResume();
        check(hit.state()==RunState::GameOver && hit.distance()==crashDistance && hit.obstacles()[0].z==crashZ
            && hit.groundPhase()==crashPhase,"Game over must freeze the world and ignore pause/resume");
        check(hit.speed()==0 && hit.currentSpeed()==crashSpeed,"Game over must stop the world and keep the crash speed");
        check(!hit.checkCollision(0,0,flat),"Game over must not report a second crash");

        const double firstCrash=hit.distance();
        check(near(hit.best(),firstCrash),"Crash must record the best score");
        hit.restart();
        check(hit.state()==RunState::Running && hit.distance()==0 && hit.groundPhase()==0 && hit.recycled()==0,
            "Restart must start a clean run");
        check(hit.speed()==Game::BaseSpeed,"Restart must restore the base speed");
        check(near(hit.obstacles()[0].z,first.z) && near(hit.best(),firstCrash),"Restart must restore obstacles and keep best");
        hit.update(0.5); hit.reset();
        check(hit.state()==RunState::Ready && near(hit.best(),firstCrash),"Shorter run must not replace best");
        hit.restart(); hit.update(10); hit.reset();
        check(near(hit.best(),90),"Longer run must replace best");
        hit.restart(); hit.update(timeToTravel(-hit.obstacles()[0].z));
        check(hit.checkCollision(0,0,flat) && near(hit.best(),90),"Later shorter crash must keep best");

        HeightField ground;
        auto run=[&](bool jump,double warmup) {
            Game g(3); g.startOrResume();
            g.update(warmup);
            Player p; p.reset(ground,0,false);
            const GroundHeight surface=[&](float x,float z) { return ground.surfaceHeight(x,z,static_cast<float>(g.groundPhase()),false); };
            check(g.obstacles()[0].x==0 && g.obstacles()[0].z<-8,"Centre obstacle must approach from ahead");
            bool jumped=false;
            for(int i=0;i<10*240 && g.state()==RunState::Running && g.obstacles()[0].z<2;++i) {
                if(jump && !jumped && g.obstacles()[0].z+g.speed()*0.4>=0) { p.jump(); jumped=true; }
                g.update(1.0/240);
                p.update(1.0f/240,0,ground,static_cast<float>(g.groundPhase()),false);
                g.checkCollision(p.x,p.feetY,surface);
            }
            return g.state();
        };
        check(run(false,0)==RunState::GameOver,"Running into the centre obstacle must end the run");
        check(run(true,0)==RunState::Running,"Jumping over the low obstacle must not collide");
        check(run(false,90)==RunState::GameOver,"Head-on crash must also happen at top speed");
        check(run(true,90)==RunState::Running,"Jumping over the low obstacle must work at top speed");
        std::cout<<"PASS: "<<checks<<" assertions; ready, speed ramp and cap, shared travel, pause, recycle, reset, 75-second FPS independence, long-run precision, invalid input, collisions, jump clearance, game over, restart, best score\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
