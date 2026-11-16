#include "SmokeTest.h"
#include "GlDiagnostics.h"
#include <GL/freeglut.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* text) { if (!condition) throw std::runtime_error(text); }
std::vector<unsigned char> readPixels(int w, int h) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(w)*h*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1); glReadBuffer(GL_BACK);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    checkGraphics("frame readback"); return pixels;
}
std::vector<std::vector<runner::Obstacle>> rowsAhead(const runner::Game& game) {
    std::vector<std::vector<runner::Obstacle>> rows;
    for (const auto& o:game.obstacles()) {
        if (!runner::Game::onTerrain(o) || o.z-o.depth*0.5f>=runner::Game::PlayerDepth*0.5f) continue;
        bool placed=false;
        for (auto& row:rows) if (std::abs(row.front().z-o.z)<0.01f) { row.push_back(o); placed=true; }
        if (!placed) rows.push_back({o});
    }
    std::sort(rows.begin(),rows.end(),[](const auto& a,const auto& b) { return a.front().z>b.front().z; });
    return rows;
}
bool laneFree(const std::vector<runner::Obstacle>& row,int lane) {
    for (const auto& o:row) if (o.x==runner::Game::laneX(lane)) return false;
    return true;
}
float autopilotMove(const runner::Game& game,float x) {
    const auto rows=rowsAhead(game);
    if (rows.empty()) return 0;
    float bestX=x,bestCost=1e9f;
    for (int lane=0;lane<3;++lane) {
        if (!laneFree(rows[0],lane)) continue;
        const float laneX=runner::Game::laneX(lane);
        float follow=0;
        if (rows.size()>1) {
            follow=1e9f;
            for (int next=0;next<3;++next)
                if (laneFree(rows[1],next)) follow=std::min(follow,std::abs(runner::Game::laneX(next)-laneX));
        }
        const float cost=std::abs(laneX-x)+0.9f*follow;
        if (cost<bestCost) { bestCost=cost; bestX=laneX; }
    }
    if (std::abs(bestX-x)<0.05f) return 0;
    return bestX>x?1.0f:-1.0f;
}
std::size_t difference(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
    require(a.size()==b.size(),"Frame sizes differ");
    std::size_t count=0;
    for (std::size_t i=0;i<a.size();i+=3)
        for (int c=0;c<3;++c) if (std::abs(int(a[i+c])-int(b[i+c]))>1) { ++count;break; }
    return count;
}
void save(const std::filesystem::path& file, int w, int h, const std::vector<unsigned char>& pixels) {
    if (!file.parent_path().empty()) std::filesystem::create_directories(file.parent_path());
    std::ofstream out(file,std::ios::binary);
    if (!out) throw std::runtime_error("Cannot save screenshot: "+file.string());
    out<<"P6\n"<<w<<' '<<h<<"\n255\n";
    for (int y=h-1;y>=0;--y) out.write(reinterpret_cast<const char*>(pixels.data()+static_cast<std::size_t>(y)*w*3),w*3);
    require(bool(out),"Screenshot write failed");
}
}

RunOptions parseRunOptions(int argc,char** argv) {
    RunOptions o;
    for (int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        auto value=[&]() -> std::string { if (++i>=argc) throw std::invalid_argument("Missing value: "+arg); return argv[i]; };
        if (arg=="--smoke-test") o.smoke=true;
        else if (arg=="--benchmark") o.benchmark=true;
        else if (arg=="--screenshot") o.screenshot=value();
        else if (arg=="--instances") {
            std::size_t used=0; const auto v=value();
            const auto n=std::stoul(v,&used);
            if (used!=v.size() || n<1 || n>10000) throw std::invalid_argument("--instances must be 1..10000");
            o.instances=n;
            o.stress=true;
        } else if (arg=="--help") {
            std::cout<<"endless_runner [--smoke-test] [--screenshot file.ppm] [--benchmark] [--instances 96]\n"
                <<"Space/Enter start; Space jump; A/D move; P pause; R reset; Enter/R restart after game over; T height map; I instancing; C camera; arrows orbit; wheel zoom; J/L light; V wireframe; N/M stress test more/fewer obstacles; B back to game; H help; Esc exit. --instances N starts the stress test with N obstacles; --benchmark measures that stress layout.\n";
            std::exit(0);
        } else throw std::invalid_argument("Unknown option: "+arg);
    }
    if (!o.screenshot.empty() && !o.smoke) throw std::invalid_argument("--screenshot requires --smoke-test");
    return o;
}

void runValidation(const RunOptions& options, Renderer& renderer,
    HeightField& terrain, Player& player, Camera& camera, runner::Game& game,
    bool& displaced, bool& instanced,
    const std::function<void(float,float)>& tick,
    const std::function<void(unsigned char)>& key,
    const std::function<std::vector<std::string>()>& hud,
    const std::function<std::vector<std::string>()>& banner) {
    auto frame=[&](bool show=true, bool overlay=false, float light=0.8f, int w=1280,int h=720) {
        const float phase=static_cast<float>(game.groundPhase());
        camera.update(0,player,0,0,terrain.surfaceHeight(player.x,0,phase,displaced));
        renderer.updateObstacles(game,terrain,displaced);
        glViewport(0,0,w,h);
        renderer.draw(w,h,camera,player,phase,displaced,light,false,overlay?hud():std::vector<std::string>{},instanced,show,
            overlay?banner():std::vector<std::string>{});
        checkGraphics("integrated frame");
    };
    auto drive=[&](float seconds) {
        const int steps=static_cast<int>(std::lround(seconds*120.0f));
        for (int i=0;i<steps;++i) tick(1.0f/120.0f,autopilotMove(game,player.x));
    };
    if (game.stress()) key('b');
    require(game.state()==runner::RunState::Ready && !game.stress(),"Program must start in Ready");
    require(!banner().empty() && banner()[0]=="ENDLESS RUNNER","Ready must show the start banner");
    for (const auto& line:hud()) require(line.find("STAGE")==std::string::npos,"HUD must not show outdated stage text");
    tick(1.0f,1);
    require(game.distance()==0 && player.x==0,"Ready must keep the world and player still");
    key('r'); require(game.state()==runner::RunState::Ready,"R in Ready must keep Ready");
    key(13); require(game.state()==runner::RunState::Running && player.grounded,"Enter must start the run");
    require(banner().empty(),"Running must hide the banner");
    drive(1.5f);
    require(game.state()==runner::RunState::Running,"Autopilot must avoid the opening rows");
    if (options.smoke) {
        renderer.validateInstanceLayout();
        frame(); const auto on=readPixels(1280,720);
        const auto count=renderer.obstacleCount();
        const auto typeDraws=renderer.obstacleDrawCalls();
        require(renderer.obstacleCount(0)>0 && renderer.obstacleCount(1)>0 && renderer.obstacleCount(2)>0,"All three obstacle types must be visible");
        require(count>0 && typeDraws==3 && renderer.expectedObstacleDraws(true)==3,"Expected one instanced draw per obstacle type");
        key('i'); require(!instanced,"I must disable instancing");
        frame(); const auto off=readPixels(1280,720);
        require(renderer.obstacleDrawCalls()==count && renderer.expectedObstacleDraws(false)==count,"Individual draw count must match uploaded instances");
        const auto mismatches=difference(on,off);
        require(mismatches==0,"Instanced and individual scenes differ");
        frame(false); const auto visible=difference(on,readPixels(1280,720));
        require(visible>100,"Obstacles must be visible above the real terrain");
        key('i');
        renderer.validateTerrainSampling(terrain,static_cast<float>(game.groundPhase()),true);
        renderer.validateTerrainSampling(terrain,127.99f,true);
        renderer.validateTerrainSampling(terrain,0.01f,true);
        key('t'); require(!displaced,"T must disable displacement");
        frame(); require(difference(on,readPixels(1280,720))>100,"Height map toggle must alter the geometry");
        renderer.validateTerrainSampling(terrain,static_cast<float>(game.groundPhase()),false);
        require(std::abs(player.feetY)<0.0001f,"Player must follow flat terrain after T");
        key('t');

        const double distance=game.distance(),phase=game.groundPhase();
        const float z=game.obstacles()[0].z,x=player.x,y=player.feetY;
        key('p'); tick(0.1f,1); key(' ');
        require(game.state()==runner::RunState::Paused && game.distance()==distance && game.groundPhase()==phase,
            "P must freeze the common clock and distance");
        require(game.obstacles()[0].z==z && player.x==x && player.feetY==y && player.grounded,
            "Pause must freeze player and obstacles and block jumping");
        key('p'); key(' '); tick(1.0f/60,0);
        require(!player.grounded && player.feetY>terrain.surfaceHeight(player.x,0,static_cast<float>(game.groundPhase()),displaced),"Space must jump");
        drive(1.5f);
        require(player.grounded && game.state()==runner::RunState::Running,"Player must land on the displaced mesh");
        const float beforeMove=player.x;
        tick(0.05f,beforeMove>0?-1.0f:1.0f); require(player.x!=beforeMove,"Player horizontal input must move");
        frame(); require(difference(on,readPixels(1280,720))>100,"Moving world must change the frame");
        auto beforeLight=readPixels(1280,720); frame(true,false,2.8f);
        require(difference(beforeLight,readPixels(1280,720))>100,"Directional light must change shading");
        key('c'); require(camera.orbit,"C must select orbit camera");
        camera.update(0.5f,player,1,0.5f,player.feetY); frame(); readPixels(1280,720);
        key('c'); require(!camera.orbit,"C must restore follow camera");

        glutReshapeWindow(960,540); glutMainLoopEvent();
        require(glutGet(GLUT_WINDOW_WIDTH)==960 && glutGet(GLUT_WINDOW_HEIGHT)==540,"Resize failed");
        frame(true,true,0.8f,960,540); readPixels(960,540);
        glutReshapeWindow(1280,720); glutMainLoopEvent();
        key('r');
        require(game.distance()==0 && game.groundPhase()==0 && game.recycled()==0
            && game.obstacles()[0].z==static_cast<float>(-runner::Game::FirstRowDistance),"R must reset distance, phase and obstacles");
        require(player.x==0 && player.grounded && std::abs(player.feetY-terrain.surfaceHeight(0,0,0,displaced))<0.0001f,
            "R must reset player on terrain");
        drive(17.0f);
        require(game.state()==runner::RunState::Running,"Lane-changing autopilot must survive 17 seconds");
        require(game.groundPhase()<HeightField::period && game.recycled()>0,"Phase and obstacle recycling must stay bounded");
        frame(); renderer.validateTerrainSampling(terrain,static_cast<float>(game.groundPhase()),displaced);
        key('p'); key('r');
        require(game.state()==runner::RunState::Paused,"Reset must preserve pause");
        key('p');
        for(int i=0;i<30*120 && game.state()==runner::RunState::Running;++i) tick(1.0f/120,0);
        require(game.state()==runner::RunState::GameOver,"Standing still must end the run");
        const double crashDistance=game.distance(); const float crashZ=game.obstacles()[0].z;
        tick(0.5f,1); key(' '); key('p');
        require(game.state()==runner::RunState::GameOver && game.distance()==crashDistance && game.obstacles()[0].z==crashZ
            && player.x==0 && player.grounded,"Game over must freeze world, player, pause and jump");
        require(game.best()>=crashDistance,"Best score must keep the crash distance");
        require(banner().size()>=4 && banner()[0]=="GAME OVER","Game over must show score, best and restart hint");
        frame(); const auto plain=readPixels(1280,720);
        renderer.draw(1280,720,camera,player,static_cast<float>(game.groundPhase()),displaced,0.8f,false,{},instanced,true,banner());
        const auto bannerPixels=difference(plain,readPixels(1280,720));
        require(bannerPixels>2000,"Game over banner must be drawn in the centre");
        key(13);
        require(game.state()==runner::RunState::Running && game.distance()==0
            && game.obstacles()[0].z==static_cast<float>(-runner::Game::FirstRowDistance) && player.x==0,"Enter must start a new run after game over");
        for(int i=0;i<30*120 && game.state()==runner::RunState::Running;++i) tick(1.0f/120,0);
        require(game.state()==runner::RunState::GameOver,"Second still run must end in game over");
        key('r');
        require(game.state()==runner::RunState::Running && game.distance()==0,"R must start a new run after game over");
        key('n');
        require(game.stress() && game.poolSize()==96 && game.state()==runner::RunState::Running,"N must enter the stress test with 96 obstacles");
        frame();
        require(renderer.obstacleCount()==96 && renderer.obstacleDrawCalls()==3,"Stress test must submit every obstacle in three draws");
        key('n'); key('n'); key('n');
        require(game.poolSize()==1000,"N must step 96, 250, 500, 1000");
        frame(); const auto stressOn=readPixels(1280,720);
        require(renderer.obstacleCount()==1000 && renderer.obstacleDrawCalls()==3,"1000 stress obstacles must use three instanced draws");
        key('i'); frame(); const auto stressOff=readPixels(1280,720);
        require(renderer.obstacleDrawCalls()==1000,"Stress test without instancing must draw every obstacle");
        const auto stressMismatch=difference(stressOn,stressOff);
        require(stressMismatch==0,"Stress instanced and individual scenes differ");
        key('i');
        const double stressStart=game.distance();
        for(int i=0;i<4*120;++i) tick(1.0f/120,0);
        frame();
        require(game.state()==runner::RunState::Running && game.distance()>stressStart+30 && renderer.obstacleCount()==1000,
            "Stress test must scroll without collisions and keep every obstacle on the terrain");
        bool stressHud=false;
        for (const auto& line:hud()) stressHud=stressHud || line.find("STRESS TEST: 1000")!=std::string::npos;
        require(stressHud,"HUD must show the stress obstacle count");
        key('m'); require(game.stress() && game.poolSize()==500,"M must reduce the stress obstacle count");
        key('b');
        require(!game.stress() && game.poolSize()==96 && game.state()==runner::RunState::Ready && game.distance()==0,
            "B must return to the normal game");
        key(13);
        drive(1.0f);
        key('p');
        frame(true,true);
        if(!options.screenshot.empty()) save(options.screenshot,1280,720,readPixels(1280,720));
        std::cout<<"PASS integrated GL smoke: "<<typeDraws<<" vs "<<count<<" obstacle draws, mismatched pixels="<<mismatches
            <<", visible obstacle pixels="<<visible
            <<"; actual terrain VS matches CPU at 17297 vertices for 5 phase/toggle cases; P/R/T/I/C, jump/landing, autopilot lane changes, light, resize, bounded scrolling, collision/game over, Ready/Enter/R restart, best score, game over banner pixels="<<bannerPixels<<", stress N/M/B with 1000 obstacles (3 vs 1000 draws, mismatched pixels="<<stressMismatch<<"); no GL errors\n";
    }
    if(options.benchmark) {
        std::cout<<"mode,pool,visible,obstacle_draws,width,height,frames,frame_ms,fps\n";
        game.configure(options.instances,true);
        player.reset(terrain,0,displaced);
        for(int i=0;i<120;++i) tick(1.0f/120,0);
        require(game.stress() && game.state()==runner::RunState::Running,"Benchmark must run the stress layout");
        for(bool mode:{true,false}) {
            instanced=mode;
            frame();
            require(renderer.obstacleCount()==game.poolSize(),"Benchmark must submit every stress obstacle");
            auto render=[&]() { renderer.draw(1280,720,camera,player,static_cast<float>(game.groundPhase()),displaced,0.8f,false,{},mode); glFinish(); };
            for(int i=0;i<20;++i) render();
            const auto start=std::chrono::steady_clock::now();
            for(int i=0;i<120;++i) render();
            const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            checkGraphics("benchmark");
            std::cout<<(mode?"ON":"OFF")<<','<<game.poolSize()<<','<<renderer.obstacleCount()<<','<<renderer.obstacleDrawCalls()
                <<",1280,720,120,"<<std::fixed<<std::setprecision(3)<<seconds*1000/120<<','<<120/seconds<<'\n';
        }
    }
}
