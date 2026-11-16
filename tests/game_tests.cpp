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
        game.update(3600);
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
        const GroundHeight flat=[](float,float) { return 0.0f; };
        const GroundHeight raised=[](float,float) { return 2.0f; };
        Game hit; hit.startOrResume();
        const auto lead=hit.obstacles()[0];
        check(lead.x==0 && near(lead.height,1.2),"Lead obstacle must sit in the centre lane");
        hit.update(-lead.z/hit.speed());
        check(near(hit.obstacles()[0].z,0),"Lead obstacle must reach the player plane");
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
        hit.update(5); hit.togglePause(); hit.startOrResume();
        check(hit.state()==RunState::GameOver && hit.distance()==crashDistance && hit.obstacles()[0].z==crashZ,
            "Game over must freeze the world and ignore pause/resume");
        check(!hit.checkCollision(0,0,flat),"Game over must not report a second crash");

        HeightField ground;
        auto run=[&](bool jump) {
            Game g; g.startOrResume();
            Player p; p.reset(ground,0,false);
            const GroundHeight surface=[&](float x,float z) { return ground.surfaceHeight(x,z,static_cast<float>(g.groundPhase()),false); };
            bool jumped=false;
            for(int i=0;i<4*240 && g.state()==RunState::Running && g.obstacles()[0].z<2;++i) {
                if(jump && !jumped && g.obstacles()[0].z+g.speed()*0.4>=0) { p.jump(); jumped=true; }
                g.update(1.0/240);
                p.update(1.0f/240,0,ground,static_cast<float>(g.groundPhase()),false);
                g.checkCollision(p.x,p.feetY,surface);
            }
            return g.state();
        };
        check(run(false)==RunState::GameOver,"Running into the centre obstacle must end the run");
        check(run(true)==RunState::Running,"Jumping over the low obstacle must not collide");
        std::cout<<"PASS: "<<checks<<" assertions; ready, motion, score, pause, recycle, reset, 75-second FPS independence, long-run precision, invalid input, collisions, jump clearance, game over\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
