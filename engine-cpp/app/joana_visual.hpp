#pragma once
#include "fighter.hpp"
#include <algorithm>
#include <cmath>

namespace KoRama {
inline float joanaVisualEase(float x) { x=std::clamp(x,0.0f,1.0f); return x*x*(3-2*x); }
inline bool joanaCrushing(const Fighter& f) {
    return f.charId==2 && f.state==FIGHTER_SKILL && f.activeSkill==SkillId::JoanaClawCrush && !f.signatureOnly;
}
// Presentation only: native damage, physics, hitboxes and move duration stay untouched.
// The core's ClawCrush damage event is elapsed frame 12. Put the visual apex there.
inline Fighter joanaVisualPose(const Fighter& f) {
    Fighter visual=f;
    if (joanaCrushing(f) && f.maxTimer>12) {
        const float elapsed=static_cast<float>(f.maxTimer-f.timer);
        const float p=elapsed<=12 ? .60f*elapsed/12 : .60f+.40f*(elapsed-12)/(f.maxTimer-12);
        visual.resetPose();
        visual.animateCommonPunch(FIGHTER_CROSS,std::clamp(p,0.0f,1.0f),false);
    }
    return visual;
}
inline int joanaJawFrame(const Fighter& f) {
    float jaw=0;
    const float p=f.maxTimer>0?1.0f-static_cast<float>(f.timer)/f.maxTimer:0;
    if (joanaCrushing(f)) {
        const float e=static_cast<float>(f.maxTimer-f.timer);
        jaw=e<10 ? -joanaVisualEase(e/7) : e<=12 ? -1+3*joanaVisualEase((e-10)/2) : 2*(1-joanaVisualEase((e-17)/8));
    } else if (f.state==FIGHTER_SKILL && f.activeSkill==SkillId::JoanaOceanBounty && !f.signatureOnly && p>=.18f && p<.78f) {
        jaw=-.65f*std::max(0.0f,std::sin((p-.18f)/.60f*3.14159265f*6));
    }
    constexpr float stages[]={-1,-.65f,-.3f,0,.4f,.8f,1.2f,1.6f,2};
    int best=0;for(int i=1;i<9;++i)if(std::abs(stages[i]-jaw)<std::abs(stages[best]-jaw))best=i;
    return best;
}
}
