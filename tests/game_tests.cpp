#include "Game.h"
#include "Player.h"
#include <algorithm>
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
static double timeAtDistance(double metres) {
    const double v=Game::BaseSpeed,a=Game::Acceleration,rampEnd=(Game::MaxSpeed-v)/a;
    const double rampDistance=Game::distanceAt(rampEnd);
    if (metres<=rampDistance) return (-v+std::sqrt(v*v+2.0*a*metres))/a;
    return rampEnd+(metres-rampDistance)/Game::MaxSpeed;
}
static void advance(Game& game,double metres) {
    game.update(timeAtDistance(game.distance()+metres)-game.elapsed());
}
static std::vector<Obstacle> rowAt(const Game& game,float z) {
    std::vector<Obstacle> row;
    for (const auto& o:game.obstacles()) if (std::abs(o.z-z)<0.01f) row.push_back(o);
    return row;
}
template <typename Predicate>
static float findRow(const Game& game,Predicate predicate) {
    float best=-std::numeric_limits<float>::infinity();
    for (const auto& o:game.obstacles())
        if (o.z<-20 && o.z>best && predicate(rowAt(game,o.z))) best=o.z;
    if (!std::isfinite(best)) throw std::runtime_error("No matching row ahead");
    return best;
}
static bool blocks(const std::vector<Obstacle>& row,int lane,int type=-1) {
    for (const auto& o:row) if (o.x==Game::laneX(lane) && (type<0 || o.type==type)) return true;
    return false;
}
static bool blocked(const RowLayout& row,int lane) {
    for (int k=0;k<row.count;++k) if (row.lane[k]==lane) return true;
    return false;
}
int main() {
    try {
        const GroundHeight flat=[](float,float) { return 0.0f; };
        const GroundHeight raised=[](float,float) { return 2.0f; };
        const float jumpPeak=7.2f*7.2f/(2.0f*18.0f);

        Game game;
        const auto initial=game.obstacles();
        const float firstZ=initial.front().z;
        game.update(10);
        check(game.state()==RunState::Ready && game.distance()==0,"Ready must not advance");
        check(game.speed()==0 && game.currentSpeed()==Game::BaseSpeed,"Ready world must stand still at the base speed");
        check(near(firstZ,-Game::FirstRowDistance),"First row starts 30 m ahead");
        game.startOrResume();
        check(game.speed()==Game::BaseSpeed,"Run must start at 8 m/s");
        game.update(1.0);
        check(near(game.distance(),8.1),"Score must integrate the accelerating speed");
        check(near(game.speed(),8.2),"Speed must grow by 0.2 m/s per second");
        check(near(game.obstacles()[0].z,firstZ+game.distance(),1e-4),"Obstacles must approach by the score travel");
        check(near(game.groundPhase(),game.distance()),"Ground phase must use the same travel");
        const double paused=game.distance();
        game.togglePause(); game.update(30);
        check(game.state()==RunState::Paused && game.speed()==0,"Pause must stop the world speed");
        check(near(game.currentSpeed(),8.2),"Pause must keep the run speed");
        check(near(game.distance(),paused),"Pause must freeze score");
        check(near(game.obstacles()[0].z,firstZ+paused,1e-4),"Pause must freeze transforms");
        game.startOrResume(); game.update(5);
        check(game.recycled()>0,"Passing rows must recycle");
        check(near(game.obstacles().back().z,initial.back().z+game.distance(),1e-3),"Unwrapped rows must move by the score travel");
        check(near(game.groundPhase(),std::fmod(game.distance(),Game::GroundPeriod),1e-6),"Phase and score must not drift");
        game.update(3600);
        check(game.rowCount()==48,"Row pool size must remain constant");
        for (const auto& o:game.obstacles())
            check(o.z<=Game::RecycleZ && o.z>=Game::RecycleZ-game.loopLength()-0.001,"Wrapped Z is bounded");
        check(game.groundPhase()>=0 && game.groundPhase()<Game::GroundPeriod,"Ground phase is bounded");
        check(game.speed()==Game::MaxSpeed,"Speed must stop at 22 m/s");
        bool regenerated=true;
        std::size_t offset=0;
        for (std::size_t r=0;r<game.rowCount();++r) {
            const RowLayout layout=Game::rowLayout(game.rowSerial(r));
            for (int k=0;k<layout.count;++k,++offset) {
                const Obstacle& o=game.obstacles()[offset];
                regenerated=regenerated && o.x==Game::laneX(layout.lane[k]) && o.type==layout.type[k];
            }
            regenerated=regenerated && game.rowSerial(r)%game.rowCount()==r;
        }
        check(regenerated && offset==game.obstacles().size(),"Recycled rows must be rebuilt from their new serial");
        game.reset();
        check(game.state()==RunState::Ready && game.distance()==0 && game.recycled()==0 && game.elapsed()==0,"Reset must clear state");
        check(game.currentSpeed()==Game::BaseSpeed,"Reset must restore the base speed");
        bool restored=game.obstacles().size()==initial.size();
        for (std::size_t i=0;restored && i<initial.size();++i)
            restored=game.obstacles()[i].x==initial[i].x && game.obstacles()[i].z==initial[i].z && game.obstacles()[i].type==initial[i].type;
        check(restored,"Reset restores the deterministic layout");

        check(Game::speedAt(0)==8 && near(Game::speedAt(35),15) && Game::speedAt(70)==22 && Game::speedAt(1e6)==22,
            "Speed ramps from 8 to 22 m/s");
        check(near(Game::speedAtDistance(Game::distanceAt(20)),Game::speedAt(20),1e-9) && Game::speedAtDistance(1e9)==22,
            "Speed by distance matches speed by time");
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

        Game twin;
        bool same=twin.obstacles().size()==initial.size();
        for (std::size_t i=0;same && i<initial.size();++i)
            same=twin.obstacles()[i].x==initial[i].x && twin.obstacles()[i].z==initial[i].z && twin.obstacles()[i].type==initial[i].type;
        check(same,"Layout must be identical for every new game");
        const std::uint64_t rows=20000;
        bool counts=true, passable=true, noRepeat=true, distinct=true, typesValid=true, pure=true;
        int typeSeen[Game::TypeCount]{};
        int earlyDouble=0, lateDouble=0;
        for (std::uint64_t n=0;n<rows;++n) {
            const RowLayout row=Game::rowLayout(n);
            counts=counts && (row.count==1 || row.count==2);
            passable=passable && row.count<3;
            if (row.count==2) distinct=distinct && row.lane[0]!=row.lane[1];
            for (int k=0;k<row.count;++k) {
                typesValid=typesValid && row.type[k]>=0 && row.type[k]<Game::TypeCount && row.lane[k]>=0 && row.lane[k]<3;
                if (row.type[k]>=0 && row.type[k]<Game::TypeCount) ++typeSeen[row.type[k]];
            }
            if (n>0) {
                const RowLayout last=Game::rowLayout(n-1);
                for (int k=0;k<row.count;++k) noRepeat=noRepeat && !blocked(last,row.lane[k]);
            }
            if (row.count==2) { if (n<60) ++earlyDouble; else if (n>=10000) ++lateDouble; }
            const RowLayout again=Game::rowLayout(n);
            pure=pure && again.count==row.count && again.lane[0]==row.lane[0] && again.type[0]==row.type[0]
                && again.lane[1]==row.lane[1] && again.type[1]==row.type[1];
        }
        check(pure,"Row layout must be a pure function of the row serial");
        check(counts && distinct && typesValid,"Every row blocks one or two different lanes with a valid type");
        check(passable,"Every row keeps a free lane");
        check(noRepeat,"Consecutive rows never block the same lane");
        check(typeSeen[0]>0 && typeSeen[1]>0 && typeSeen[2]>0,"All three obstacle types are generated");
        check(earlyDouble/60.0<lateDouble/10000.0,"Two-lane rows must become more frequent at higher speed");
        check(Game::doubleRowChance(1)<Game::doubleRowChance(41) && Game::doubleRowChance(41)<Game::doubleRowChance(81)
            && near(Game::doubleRowChance(1001),Game::doubleRowChance(2001)),"Two-lane chance follows the speed ramp and caps");
        bool reachable=true;
        bool lanes[3]{};
        for (int lane=0;lane<3;++lane) lanes[lane]=!blocked(Game::rowLayout(0),lane);
        for (std::uint64_t n=1;n<rows && reachable;++n) {
            const RowLayout row=Game::rowLayout(n);
            bool next[3]{};
            for (int lane=0;lane<3;++lane) {
                const bool fromNear=lanes[lane] || (lane>0 && lanes[lane-1]) || (lane<2 && lanes[lane+1]);
                next[lane]=fromNear && !blocked(row,lane);
            }
            reachable=next[0] || next[1] || next[2];
            std::copy(next,next+3,lanes);
        }
        check(reachable,"A path needing at most one lane change between rows always exists");

        Game slow,fast,single;
        slow.startOrResume(); fast.startOrResume(); single.startOrResume();
        for(int i=0;i<30*75;++i) slow.update(1.0/30.0);
        for(int i=0;i<144*75;++i) fast.update(1.0/144.0);
        single.update(75);
        check(near(slow.distance(),fast.distance(),1e-7) && near(slow.distance(),single.distance(),1e-7),"Distance must not depend on frame rate");
        check(near(slow.currentSpeed(),fast.currentSpeed(),1e-9) && fast.currentSpeed()==Game::MaxSpeed,"Speed must not depend on frame rate");
        check(near(slow.groundPhase(),fast.groundPhase(),1e-6),"Ground phase must not depend on frame rate");
        check(slow.recycled()>0 && slow.recycled()==fast.recycled() && slow.recycled()==single.recycled(),"Recycling must not depend on frame rate");
        check(slow.obstacles().size()==fast.obstacles().size() && slow.obstacles().size()==single.obstacles().size(),"Layout size must not depend on frame rate");
        for(std::size_t i=0;i<slow.obstacles().size();++i) {
            const auto &a=slow.obstacles()[i], &b=fast.obstacles()[i], &c=single.obstacles()[i];
            check(near(a.z,b.z,0.0001) && a.x==b.x && a.type==b.type,"Movement and layout must not depend on frame rate");
            check(near(a.z,c.z,0.0001) && a.x==c.x && a.type==c.type,"Movement and layout must not depend on step size");
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

        const Obstacle low=Game::makeObstacle(0,0,0), crate=Game::makeObstacle(1,1,0), column=Game::makeObstacle(2,2,0);
        check(low.height<=0.9f && low.height<jumpPeak && crate.height>jumpPeak && column.height>crate.height,
            "Only the low barrier is lower than the jump peak");
        check(low.width>=Game::LaneSpacing-Game::PlayerWidth && Game::laneX(0)==-3.2f && Game::laneX(2)==3.2f,
            "Obstacles fill their 3.2 m lanes");
        check(Game::onTerrain({0,0,1,1,1,0}) && !Game::onTerrain({0,-170,1,1,1,0}) && !Game::onTerrain({0,15.5f,1,1,1.35f,0}),
            "Only obstacles drawn on the terrain may collide");

        Game hit; hit.startOrResume();
        const float centreZ=findRow(hit,[](const std::vector<Obstacle>& row) { return row.size()==1 && blocks(row,1); });
        advance(hit,-centreZ);
        check(rowAt(hit,0).size()==1 && blocks(rowAt(hit,0),1),"Single centre row must reach the player");
        const float clear=Game::ObstacleWidth*0.5f+Game::PlayerWidth*0.5f;
        check(hit.collides(0,0,flat),"Head-on overlap must collide");
        check(!hit.collides(clear+0.01f,0,flat) && !hit.collides(-clear-0.01f,0,flat),"Free neighbour lanes must not collide");
        check(hit.collides(clear-0.01f,0,flat) && hit.collides(-clear+0.01f,0,flat),"Side overlap must collide");
        check(!hit.collides(0,0,raised),"Obstacle base must follow the ground height");
        check(hit.collides(0,2.5f,raised),"Raised obstacle must collide with raised player");
        Game far; far.startOrResume();
        check(!far.collides(0,0,flat),"Distant obstacles must not collide");

        Game pair; pair.startOrResume();
        const float pairZ=findRow(pair,[](const std::vector<Obstacle>& row) { return row.size()==2 && std::abs(row[0].x-row[1].x)<4; });
        advance(pair,-pairZ);
        const auto adjacent=rowAt(pair,0);
        const float between=0.5f*(adjacent[0].x+adjacent[1].x);
        check(pair.collides(between,0,flat),"No safe gap between two blocked lanes");
        check(pair.collides(adjacent[0].x,0,flat) && pair.collides(adjacent[1].x,0,flat),"Both blocked lanes collide");

        bool trapped=true;
        for (float x=-4.2f;x<=4.2001f;x+=0.1f) {
            Game still; still.startOrResume();
            for (int i=0;i<60*120 && still.state()==RunState::Running;++i) {
                still.update(1.0/60);
                still.checkCollision(x,0,flat);
            }
            trapped=trapped && still.state()==RunState::GameOver;
        }
        check(trapped,"Standing still anywhere must end the run within two minutes");

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
        check(near(hit.obstacles()[0].z,firstZ) && near(hit.best(),firstCrash),"Restart must restore obstacles and keep best");
        hit.update(0.5); hit.reset();
        check(hit.state()==RunState::Ready && near(hit.best(),firstCrash),"Shorter run must not replace best");
        hit.restart(); hit.update(30); hit.reset();
        check(near(hit.best(),330),"Longer run must replace best");
        hit.restart(); advance(hit,-centreZ);
        check(hit.checkCollision(0,0,flat) && near(hit.best(),330),"Later shorter crash must keep best");

        HeightField ground;
        auto jumpOver=[&](int type,bool jump,double warmup) {
            Game g; g.startOrResume();
            g.update(warmup);
            const float z=findRow(g,[&](const std::vector<Obstacle>& row) {
                for (const auto& o:row) if (o.type==type) return true;
                return false;
            });
            float x=0;
            for (const auto& o:rowAt(g,z)) if (o.type==type) x=o.x;
            advance(g,-12.0-z);
            Player p; p.reset(ground,0,false);
            p.x=x;
            const GroundHeight surface=[&](float px,float pz) { return ground.surfaceHeight(px,pz,static_cast<float>(g.groundPhase()),false); };
            bool jumped=false;
            float rowZ=-12;
            for(int i=0;i<10*240 && g.state()==RunState::Running && rowZ<3;++i) {
                if(jump && !jumped && rowZ+g.speed()*0.4>=0) { p.jump(); jumped=true; }
                const double before=g.distance();
                g.update(1.0/240);
                rowZ+=static_cast<float>(g.distance()-before);
                p.update(1.0f/240,0,ground,static_cast<float>(g.groundPhase()),false);
                g.checkCollision(p.x,p.feetY,surface);
            }
            return g.state();
        };
        check(jumpOver(0,false,0)==RunState::GameOver,"Running into a low barrier must end the run");
        check(jumpOver(0,true,0)==RunState::Running,"Jumping over a low barrier must not collide");
        check(jumpOver(0,true,90)==RunState::Running,"Jumping over a low barrier must work at top speed");
        check(jumpOver(1,true,0)==RunState::GameOver,"A crate cannot be jumped");
        check(jumpOver(2,true,0)==RunState::GameOver,"A column cannot be jumped");
        check(jumpOver(2,true,90)==RunState::GameOver,"A column cannot be jumped at top speed");
        std::cout<<"PASS: "<<checks<<" assertions; ready, speed ramp and cap, shared travel, pause, recycle, reset, deterministic rows, passable lanes, no gaps, three types, jump rules, difficulty, FPS independence, long-run precision, invalid input, collisions, game over, restart, best score\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
