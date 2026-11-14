#ifndef KORAMA_SKILL_POSES_HPP
#define KORAMA_SKILL_POSES_HPP

#include "fighter.hpp"
#include "skills.hpp"

namespace KoRama {

// Pose da habilidade no progresso p (0..1). Devolve false para golpes sem pose dedicada
// (o motor usa a pose genérica).
bool animateSkillPose(Fighter& f, SkillId id, float p);

} // namespace KoRama

#endif
