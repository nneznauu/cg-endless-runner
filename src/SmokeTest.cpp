#include "SmokeTest.h"
#include "GlDiagnostics.h"
#include <GL/freeglut.h>
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
        } else if (arg=="--help") {
            std::cout<<"endless_runner [--smoke-test] [--screenshot file.ppm] [--benchmark] [--instances 96]\n"
                <<"Space jump; A/D move; P pause; R reset; T height map; I instancing; C camera; arrows orbit; wheel zoom; J/L light; V wireframe; H help; Esc exit.\n";
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
    const std::function<std::vector<std::string>()>& hud) {
    auto frame=[&](bool show=true, bool overlay=false, float light=0.8f, int w=1280,int h=720) {
        const float phase=static_cast<float>(game.groundPhase());
        camera.update(0,player,0,0,terrain.surfaceHeight(player.x,0,phase,displaced));
        renderer.updateObstacles(game,terrain,displaced);
        glViewport(0,0,w,h);
        renderer.draw(w,h,camera,player,phase,displaced,light,false,overlay?hud():std::vector<std::string>{},instanced,show);
        checkGraphics("integrated frame");
    };
    for (int i=0;i<60;++i) tick(1.0f/60.0f,0);
    if (options.smoke) {
        renderer.validateInstanceLayout();
        frame(); const auto on=readPixels(1280,720);
        const auto count=renderer.obstacleCount();
        require(count>0 && renderer.obstacleDrawCalls()==1,"Expected one instanced draw");
        key('i'); require(!instanced,"I must disable instancing");
        frame(); const auto off=readPixels(1280,720);
        require(renderer.obstacleDrawCalls()==count,"Individual draw count must match uploaded instances");
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
        for (int i=0;i<180;++i) tick(1.0f/120,0);
        require(player.grounded,"Player must land on the displaced mesh");
        tick(0.05f,1); require(player.x>0,"Player horizontal input must move");
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
        require(game.distance()==0 && game.groundPhase()==0 && game.recycled()==0 && game.obstacles()[0].z==-18,
            "R must reset distance, phase and obstacles");
        require(player.x==0 && player.grounded && std::abs(player.feetY-terrain.surfaceHeight(0,0,0,displaced))<0.0001f,
            "R must reset player on terrain");
        // Cross the 128 m wrap and several obstacle recycling events.
        for (int i=0;i<17*120;++i) tick(1.0f/120,0);
        require(game.groundPhase()<HeightField::period && game.recycled()>0,"Phase and obstacle recycling must stay bounded");
        frame(); renderer.validateTerrainSampling(terrain,static_cast<float>(game.groundPhase()),displaced);
        key('p'); key('r');
        require(game.state()==runner::RunState::Paused,"Reset must preserve pause");
        key('p'); for(int i=0;i<120;++i) tick(1.0f/120,0); key('p');
        frame(true,true);
        if(!options.screenshot.empty()) save(options.screenshot,1280,720,readPixels(1280,720));
        std::cout<<"PASS integrated GL smoke: 1 vs "<<count<<" obstacle draws, mismatched pixels="<<mismatches
            <<", visible obstacle pixels="<<visible
            <<"; actual terrain VS matches CPU at 17297 vertices for 5 phase/toggle cases; P/R/T/I/C, jump/landing, movement, light, resize, bounded scrolling; no GL errors\n";
    }
    if(options.benchmark) {
        std::cout<<"mode,pool,visible,obstacle_draws,width,height,frames,frame_ms,fps\n";
        key('r'); if(game.state()!=runner::RunState::Running) key('p');
        for(int i=0;i<120;++i) tick(1.0f/120,0);
        for(bool mode:{true,false}) {
            instanced=mode;
            // Use the same prepared buffers and scene; excludes update/upload/HUD.
            frame();
            auto render=[&]() { renderer.draw(1280,720,camera,player,static_cast<float>(game.groundPhase()),displaced,0.8f,false,{},mode); glFinish(); };
            for(int i=0;i<20;++i) render();
            const auto start=std::chrono::steady_clock::now();
            for(int i=0;i<120;++i) render();
            const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            checkGraphics("benchmark");
            std::cout<<(mode?"ON":"OFF")<<','<<game.obstacles().size()<<','<<renderer.obstacleCount()<<','<<renderer.obstacleDrawCalls()
                <<",1280,720,120,"<<std::fixed<<std::setprecision(3)<<seconds*1000/120<<','<<120/seconds<<'\n';
        }
    }
}
