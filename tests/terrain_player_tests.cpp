#include "HeightField.h"
#include "Player.h"
#include "Game.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static int checks=0;
static void check(bool ok,const char* text) { ++checks; if(!ok) throw std::runtime_error(text); }
static bool near(float a,float b,float epsilon=0.0001f) { return std::abs(a-b)<epsilon; }
int main() {
    try {
        HeightField terrain;
        check(terrain.texels().size()==128*128,"Real 128x128 height data required");
        for(float phase:{0.0f,37.25f,127.99f}) for(float x:{-12.0f,-3.2f,0.0f,4.2f,12.0f}) for(float z:{-150.0f,-9.25f,0.0f,15.0f}) {
            check(near(terrain.sample(x,z,phase,true),terrain.sample(x,z,phase+128,true)),"Height texture must repeat at 128 metres");
            check(terrain.surfaceHeight(x,z,phase,false)==0,"Displacement OFF must be flat");
            check(std::isfinite(terrain.surfaceHeight(x,z,phase,true)),"Finite surface required");
        }
        // Independent barycentric weights of the two actual index-buffer triangles.
        for(float phase:{0.0f,42.5f,127.9f}) {
            const float a=terrain.sample(0,-10,phase,true),b=terrain.sample(.5f,-10,phase,true);
            const float c=terrain.sample(0,-9.5f,phase,true),d=terrain.sample(.5f,-9.5f,phase,true);
            check(near(terrain.surfaceHeight(.1f,-9.9f,phase,true),.6f*a+.2f*b+.2f*c),"First triangle interpolation mismatch");
            check(near(terrain.surfaceHeight(.4f,-9.6f,phase,true),.2f*b+.2f*c+.6f*d),"Second triangle interpolation mismatch");
        }
        Player player; runner::Game game; game.startOrResume();
        player.reset(terrain,0,true); player.jump();
        float highest=player.feetY;
        for(int i=0;i<240;++i) {
            game.update(1.0/120);
            player.update(1.0f/120,0,terrain,static_cast<float>(game.groundPhase()),true);
            highest=std::max(highest,player.feetY);
        }
        check(highest>terrain.surfaceHeight(0,0,0,true)+1.0f,"Jump must have visible height");
        check(player.grounded,"Player must land");
        check(near(player.feetY,terrain.surfaceHeight(player.x,0,static_cast<float>(game.groundPhase()),true)),"Player feet must match the visible mesh");
        player.update(10,1,terrain,0,true); check(near(player.x,4.2f),"Right movement bound");
        player.update(10,-1,terrain,0,true); check(near(player.x,-4.2f),"Left movement bound");
        player.update(0,0,terrain,0,false); check(player.feetY==0,"Toggle flat ground must reposition grounded player");
        std::cout<<"PASS terrain/player: "<<checks<<" assertions\n";return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n';return 1; }
}
