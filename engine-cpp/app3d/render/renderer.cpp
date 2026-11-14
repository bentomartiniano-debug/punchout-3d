// app3d/render/renderer.cpp
#define GLM_ENABLE_EXPERIMENTAL
#include "render/renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
namespace KoRama {
static constexpr float kCamHeight = 2.6f, kCamBack = 4.4f, kCamLookY = 0.62f, kCamLookZ = 1.25f, kCamSide = 1.0f;
static constexpr float kFovHalf = 0.39f, kLens = 1.38f;   // lente mais longa, como o alvo: os dois lutadores ficam mais próximos de tamanho
static constexpr float kFlapLegR = 0.12f, kFlapLen = 0.36f;   // aba do cinto: grossura da coxa e comprimento da aba (metros, no modelo)
static constexpr float kDotPx = 6.0f;   // retícula: tamanho da célula em pixels
static constexpr float kHatchNear = 3.4f;   // metros: mais perto da câmera que isso, o cenário e as cordas ficam hachurados
static constexpr int kShadowStatic = 2048, kShadowDynamic = 1024; static constexpr float kShadowStaticR = 24.f, kShadowDynamicR = 5.5f;
static const char* kToonVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=2) in vec2 aU; layout(location=3) in vec3 aC; layout(location=4) in vec3 aP2; layout(location=5) in vec3 aN2; layout(location=6) in vec2 aS;
uniform mat4 uVP, uM, uLS, uLD; uniform mat3 uNM; uniform vec3 uEye; uniform float uMorph, uT; uniform float uPush;   // rastro: empurrado para trás ao longo do raio da câmera (fica sempre atrás do lutador na tela)
uniform mat4 uBend; uniform int uBendOn, uScene; uniform vec3 uSect[10];
uniform vec3 uFoot[2]; vec3 footShift(vec3 p){ return mix(uFoot[0], uFoot[1], smoothstep(-0.06,0.06,p.x)); }   // pés virtuais: cada lado da malha de baixo vai até o seu pé
vec3 sceneAnim(vec3 p, vec2 s){ if(s.x<=0.0) return p; float k = floor(s.y), ph = fract(s.y)*6.2831853;
  if(k<1.5){ float a = uT*1.6+ph+p.x*1.3+p.z*0.9; p.x += sin(a)*s.x; p.z += sin(a*0.8+1.7)*s.x*0.6; }
  else if(k<2.5){ float a = uT*5.5+ph+(p.x+p.z)*5.0; p.x += sin(a)*s.x*0.6; p.z += cos(a*0.9)*s.x; p.y += sin(a*1.3)*s.x*0.3; }
  else if(k<3.5){ p.y += (sin(p.x*1.2+uT*1.9)+0.7*sin(p.z*1.6-uT*1.4))*s.x; }
  else if(k<4.5){ p.y += sin(uT*1.3+ph)*s.x; p.x += sin(uT*0.9+ph)*s.x*0.25; }
  else if(k<5.5){ float w = max(sin(uT*2.2-ph),0.0); p.y += w*w*0.07+abs(sin(uT*7.0+ph*5.0))*0.012; }
  else if(k<7.5){ float a = uT*0.8+ph; p.x += sin(a)*s.x; p.z += sin(a*2.0)*s.x*0.5; p.y += sin(a*2.3)*0.17; }
  return p; }
vec3 swayed(vec3 p, vec2 s){ if(uScene==1) return sceneAnim(p,s); if(uBendOn==1) return mix(p + footShift(p), (uBend*vec4(p,1.0)).xyz, s.x); if(s.x<=0.0) return p;
  float a = atan(p.z,p.x), f = (a+3.14159265)/6.2831853*10.0-0.5, fl = floor(f); vec3 q = mix(uSect[int(mod(fl,10.0))], uSect[int(mod(fl+1.0,10.0))], f-fl)*s.x; q.y *= 0.4;
  vec2 r = vec2(cos(a),sin(a)); q.xz += r*sin(uT*2.2+a*4.0+s.x*3.0)*0.006*s.x; float inw = dot(q.xz,r); if(inw<0.0) q.xz -= r*inw*0.75; q.y += abs(inw)*0.15*s.x; return p+q; }
uniform int uDripMode; uniform vec4 uDrip[12]; uniform vec3 uDripK;
float dripD(vec3 pp){ float th = atan(pp.z,pp.x-uDripK.x), rr = length(vec2(pp.x-uDripK.x,pp.z)), h = 0.0;
  for(int i=0;i<12;i++){ vec4 d = uDrip[i]; if(d.z<0.3) continue; float dth = mod(th-d.x+3.14159265,6.2831853)-3.14159265, ds = dth*rr, dy = pp.y-d.y, R = d.z, b = 1.0-(ds*ds+dy*dy*0.55)/(R*R), hb = b>0.0 ? sqrt(b)*R*0.65 : 0.0, ht = 0.0;
    if(dy<0.0 && dy>-d.w){ float k = 1.0+dy/d.w, w = R*0.62*k+0.6, q = 1.0-(ds/w)*(ds/w); if(q>0.0) ht = sqrt(q)*R*0.38*k; } h = max(h,max(hb,ht)); } return h; }
float dripH(vec3 p){ float th = atan(p.z,p.x), v = p.y, cr = sqrt(max(1.0-v*v,0.04)), h = 0.0;
  for(int i=0;i<12;i++){ vec4 d = uDrip[i]; if(d.z<0.02) continue; float dth = mod(th-d.x+3.14159265,6.2831853)-3.14159265, ds = dth*cr, dv = v-d.y, R = d.z, b = 1.0-(ds*ds+dv*dv)/(R*R), hb = b>0.0 ? sqrt(b)*R*0.7 : 0.0, ht = 0.0;
    if(dv>0.0 && dv<d.w){ float k = 1.0-dv/d.w, w = R*0.6*k+0.02, q = 1.0-(ds/w)*(ds/w); if(q>0.0) ht = sqrt(q)*R*0.4*k; } h = max(h,max(hb,ht)); } return h; }
vec3 pxOf(vec3 p){ return vec3(p.x*uDripK.y+uDripK.x, uDripK.z-p.y*uDripK.y, p.z*uDripK.y); }
float dripDisp(vec3 p){ if(uDripMode==0) return 0.0; return uDripMode==3 ? dripH(normalize(p)) : 1.7*dripD(pxOf(p))/uDripK.y; }
out vec3 vN, vC, vW; out vec2 vU; out float vNz, vDist, vK, vDrip; out vec4 vLS, vLD;
void main(){ vec3 N0 = mix(aN,aN2,uMorph); vN = uNM * N0; vNz = N0.z; vU = aU; vC = aC; vK = uScene==1 && aS.x>0.0 ? floor(aS.y) : 0.0;
  if(vK>7.5){ float f = fract(aS.y), d = uT*aS.x*(fract(f*2.0)<0.5 ? 1.0 : -1.0); vU += f<0.5 ? vec2(0.0,d) : vec2(d,0.0); }
   float dd = dripDisp(aP); vDrip = uDripMode==3 ? dd*30.0 : dd*uDripK.y/1.7; vec4 w = uM * vec4(swayed(mix(aP,aP2,uMorph),aS)+normalize(N0)*dd,1.0); vW = w.xyz; vDist = distance(w.xyz, uEye);
  vec4 off = vec4(w.xyz + normalize(vN) * 0.035, 1.0); vLS = uLS * off; vLD = uLD * off; gl_Position = uVP * vec4(w.xyz + normalize(w.xyz-uEye)*uPush, 1.0); })";
// luz do alvo (docs/superpowers/refs/korama3d-alvo-visual.webp): passagem suave da luz para a sombra, sombra projetada, sol quente, tinta no chão e o clarão do golpe
static const char* kToonFS = R"(#version 330 core
in vec3 vN, vC, vW; in vec2 vU; in float vNz, vDist, vK, vDrip; in vec4 vLS, vLD; out vec4 o; uniform float uT;
uniform vec4 uColor, uFlash; uniform vec3 uSun, uTint, uFog, uEye, uFlashCol; uniform vec2 uFogR; uniform sampler2D uTex, uTex2, uTexB; uniform float uTexMix; uniform sampler2DShadow uShS, uShD; uniform int uHasTex, uHasBack, uVCol, uUnlit; uniform float uRim, uPaint, uHatchR;
void hatch(float k){ if(k<0.999 && fract((gl_FragCoord.x+gl_FragCoord.y)/7.0) > mix(0.5,1.0,k)) discard; }
const vec2 kTap[4] = vec2[4](vec2(-0.7,-0.3), vec2(0.3,-0.7), vec2(0.7,0.3), vec2(-0.3,0.7));
float sun(sampler2DShadow m, vec4 q, float r){ vec3 u = q.xyz*0.5+0.5; if(u.x<0.0||u.x>1.0||u.y<0.0||u.y>1.0||u.z>1.0) return 1.0; if(r<=0.0) return texture(m,u); return (texture(m,vec3(u.xy+kTap[0]*r,u.z))+texture(m,vec3(u.xy+kTap[1]*r,u.z))+texture(m,vec3(u.xy+kTap[2]*r,u.z))+texture(m,vec3(u.xy+kTap[3]*r,u.z)))*0.25; }
uniform sampler2D uNoise; float noise(vec2 p){ return texture(uNoise,p/64.0).r; }
uniform vec4 uSmA, uSmB; uniform float uSmC;
uniform float uPz; uniform int uPzKind; uniform mat4 uPzInv; uniform float uShade, uShadeY;   // uShade: sombra do capuz (melaço) nas peças de pele acima de uShadeY (altura no modelo)
uniform vec4 uCloud; uniform float uCloudK; uniform vec3 uCloudSil;   // nuvem de pólen: esfera (centro, raio) e densidade; quem está atrás dela vira silhueta hachurada   // Pele Venenosa do King Frog (vinil): pele escura com a espiral lenta de manchas no peito e na cabeça
// rastro de velocidade rasgado como labareda: a partir da frente, o corpo esticado é cortado em línguas de comprimentos desiguais que tremulam
void smearCut(vec3 w){ if(uSmA.w<=0.0) return; vec3 r = w-uSmA.xyz, b = uSmB.xyz; float al = dot(r,b)/uSmA.w; if(uSmC<0.0 && al<0.12) return; if(al<uSmC) discard;
  vec3 a1 = normalize(cross(b, abs(b.y)<0.9 ? vec3(0,1,0) : vec3(1,0,0))), a2 = cross(b,a1); vec2 pp = vec2(dot(r,a1),dot(r,a2))*uSmB.w;
  float n = noise(vec2(pp.x*1.9 + sin(al*5.0+uT*9.0)*0.25, pp.y*1.9 + uT*1.1)), len = 0.12 + 0.95*pow(smoothstep(0.3,0.78,n),1.6);
  if(al > len) discard; }
// relevo desenhado por material (madeira, pedra, areia, folhagem): uma altura procedural no mundo vira normal pelas derivadas da tela,
// então a faixa de luz e sombra do cel shading fica irregular como pintura; a cor também ganha veio, pinta e grão
uniform int uSurf; uniform vec3 uGrain; uniform float uDots;
float tri(vec3 p, vec3 w){ return noise(p.yz)*w.x + noise(p.xz)*w.y + noise(p.xy)*w.z; }
vec3 bump(vec3 n, float h, float k){ vec3 px = dFdx(vW), py = dFdy(vW); float hx = dFdx(h), hy = dFdy(h); vec3 r1 = cross(py,n), r2 = cross(n,px); float det = dot(px,r1); if(abs(det)<1e-9) return n; vec3 g = (hx*r1+hy*r2)/det; return normalize(n - k*g); }
void surface(inout vec3 c, inout vec3 n){ if(uSurf==0) return; float far = 1.0-smoothstep(22.0,55.0,vDist); if(far<=0.0) return;
  vec3 w = pow(abs(n),vec3(4.0)); w /= w.x+w.y+w.z; float h = 0.5, k = 0.0, tone = 1.0;
  if(uSurf==1){ vec3 g = uGrain, a1 = normalize(cross(g, abs(g.y)<0.9 ? vec3(0,1,0) : vec3(1,0,0))), a2 = cross(g,a1); float al = dot(vW,g), ac = dot(vW,a1)+dot(vW,a2)*0.7;
    float ring = 0.5+0.5*sin(ac*42.0 + noise(vec2(al*1.3, ac*3.0))*7.0); h = ring*ring*0.65 + noise(vec2(al*2.5, ac*70.0))*0.35; k = 0.012; tone = mix(0.84,1.05,h); }
  else if(uSurf==2){ float big = tri(vW*2.6,w), crack = 1.0-smoothstep(0.0,0.035,abs(tri(vW*1.3+7.0,w)-0.5)); h = big*0.6 + tri(vW*9.0,w)*0.3 + tri(vW*24.0,w)*0.1 - crack*0.45; k = 0.03; tone = 0.9 + 0.16*h - crack*0.12; }
  else if(uSurf==3){ float grain = tri(vW*38.0,w), rip = 0.5+0.5*sin(vW.x*8.0+vW.z*4.5+tri(vW*1.4,w)*6.0); h = grain*0.45 + rip*0.55; k = 0.008; tone = 0.95 + 0.08*grain + 0.04*rip; }
  else { h = tri(vW*11.0,w)*0.7 + tri(vW*27.0,w)*0.3; k = 0.012; tone = 0.88 + 0.22*h; }
  c *= mix(1.0, tone, far); n = bump(n, h, k*far); }
void main(){ smearCut(vW); vec4 a = uColor;
  // pintura da frente e das costas: misturadas aos poucos nas laterais (um corte seco em vNz=0 virava uma listra vertical dos lados)
  // e na lateral a pintura fica esticada (cada pixel da beirada vira um risco comprido): ali ela é lida desfocada
  if(uHasTex==1){ float sideB = uHasBack==1 ? 3.5*(1.0-smoothstep(0.08,0.4,abs(vNz))) : 0.0;
    vec4 tx = uHasBack==1 ? mix(texture(uTex2,vU,sideB), texture(uTex,vU,sideB), smoothstep(-0.3,0.3,vNz)) : texture(uTex,vU); if(uTexMix>0.0) tx = mix(tx, texture(uTexB,vU), uTexMix); a *= tx; }   // fade entre duas expressões
  if(uVCol==1) a.rgb *= vC; if(a.a<0.05) discard;
  hatch(smoothstep(uHatchR-0.8,uHatchR,vDist));
  vec3 c = a.rgb, n = normalize(vN); if(!gl_FrontFacing) n = -n;
  if(uShade>0.0 && uPzKind==7){ vec3 pp = (uPzInv*vec4(vW,1.0)).xyz; float k = smoothstep(uShadeY-0.006, uShadeY+0.006, pp.y); c = mix(c, vec3(0.05,0.08,0.15), 0.92*uShade*k); }
  if(uPz>0.0 && uPzKind>0){
    if(uPzKind<=2){ vec3 tint = vec3(0.3,0.42,0.6);
      if(uPzKind==1){ vec3 pp = (uPzInv*vec4(vW,1.0)).xyz; vec2 q = pp.y>1.42 ? (pp.xy-vec2(0.0,1.62))/vec2(0.5,0.4) : (pp.xy-vec2(0.0,1.08))/vec2(0.5,0.4);
        float A = atan(q.y,q.x), R = length(q)*(1.0+0.2*sin(A*3.0+uT*0.3)), B = floor(R*9.0+A/6.283*2.0-uT*0.6), M = mod(B,3.0);
        tint *= M<1.0 ? vec3(0.45,0.62,0.45) : M<2.0 ? vec3(0.72,0.9,0.7) : vec3(1.0); }
      c *= mix(vec3(1.0), tint, uPz); }
    else if(uPzKind==3) c = mix(c, vec3(0.0137,0.0423,0.0152), uPz);
    else if(uPzKind==4) c = mix(c, vec3(0.323,0.235,0.036), uPz);
    else if(uPzKind==5) c = mix(c, vec3(0.005,0.479,0.222), uPz);
    else if(uPzKind==6) c = mix(c, vec3(0.068,0.716,0.216)*max(c.r,max(c.g,c.b)), uPz);
    else if(uPzKind==7 || uPzKind==9){ float l = dot(c, vec3(0.3,0.59,0.11)); c = mix(c, vec3(1.0,0.36,0.03)*min(1.05, l*1.7), uPz); } }   // Pele de Mel do Jack: laranja de mel (os traços do rosto continuam)   // Pele de Mel do Jack: âmbar (os traços do rosto continuam)
  surface(c, n);
  if(uUnlit==0){ float d = dot(n,uSun), lam = smoothstep(-0.02,0.42,d), lit = lam>0.0 ? lam * (vDist>14.0 ? sun(uShS,vLS,0.0) : min(sun(uShS,vLS,0.0011), sun(uShD,vLD,0.0032))) : 0.0, hemi = mix(0.86,1.05,n.y*0.5+0.5);
    c *= mix(uTint*0.6*hemi, vec3(0.98,0.94,0.87), lit);
    // retícula de quadrinho na sombra: pontos em grade a 45 graus que crescem conforme escurece
    if(uDots>0.0){ float sh = smoothstep(0.5,0.92,1.0-lit); if(sh>0.0){ vec2 q = mat2(0.7071,-0.7071,0.7071,0.7071)*gl_FragCoord.xy/uDots, f = fract(q)-0.5; float r = 0.12+0.24*sh, dt = 1.0-smoothstep(r-0.07,r+0.07,length(f)); c *= mix(1.0, 0.88, dt*sh); } }
    c += a.rgb * vec3(1.0,0.8,0.56) * uRim * pow(1.0-max(dot(n,normalize(uEye-vW)),0.0),3.0) * (0.35+0.65*lit); }
  if(uPaint>0.0) c *= 1.0 + uPaint * (noise(vW.xz*1.3+vW.y)+0.5*noise(vW.xz*4.1-vW.y)-0.75) * 0.09;
  // gota do creme: sombra na borda e brilho no alto
  if(vDrip>0.05){ float e = smoothstep(0.05,1.2,vDrip), top = smoothstep(2.2,4.2,vDrip); c *= mix(1.0,0.86,e*(1.0-top)); c = mix(c, c*1.14+vec3(0.04,0.035,0.025), top); }
  // água: faixas de brilho correndo, em corte duro como o resto do desenho
  if(vK>2.5 && vK<3.5){ float q = noise(vW.xz*0.9+vec2(uT*0.12,uT*0.09))+0.5*noise(vW.xz*2.3-vec2(uT*0.2,-uT*0.16)), l = 1.0-smoothstep(0.0,0.035,abs(q-0.72)), l2 = 1.0-smoothstep(0.0,0.03,abs(q-0.95)); c += vec3(0.16,0.2,0.2)*max(l,l2*0.7); }
  if(uFlash.w>0.0){ vec3 L = uFlash.xyz-vW; float q = dot(L,L); c += a.rgb * uFlashCol * uFlash.w * (0.25+0.75*max(dot(n,normalize(L)),0.0)) / (1.0+q*4.5); }
  // atrás da nuvem: o quanto do caminho do olho até aqui passa por dentro dela escurece até a silhueta, com hachura diagonal de nanquim
  if(uCloudK>0.0){ vec3 rd = vW-uEye; float L = length(rd); rd /= L; vec3 oc = uEye-uCloud.xyz; float b = dot(oc,rd), h = b*b-(dot(oc,oc)-uCloud.w*uCloud.w);
    if(h>0.0){ h = sqrt(h); float seg = max(min(-b+h,L)-max(-b-h,0.0),0.0)/(2.0*uCloud.w), cov = clamp(seg*1.7,0.0,1.0)*uCloudK, k = smoothstep(0.1,0.5,cov);
      c = mix(c, uCloudSil+c*0.06, k); if(fract((gl_FragCoord.x+gl_FragCoord.y)/6.0)<0.38) c = mix(c, uCloudSil*0.3, k*0.85); } }
  c = pow(max(c,vec3(0.0)), vec3(1.0/2.2)); o = vec4(mix(c, uFog, smoothstep(uFogR.x,uFogR.y,vDist)*0.6), a.a); })";
static const char* kHullVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=4) in vec3 aP2; layout(location=5) in vec3 aN2; layout(location=6) in vec2 aS; uniform mat4 uVP, uM; uniform vec3 uEye; uniform float uW, uFadeA, uFadeB, uMorph, uT; uniform int uFade; out float vDist; out vec3 vHW; uniform float uPush;
uniform mat4 uBend; uniform int uBendOn, uScene; uniform vec3 uSect[10];
uniform vec3 uFoot[2]; vec3 footShift(vec3 p){ return mix(uFoot[0], uFoot[1], smoothstep(-0.06,0.06,p.x)); }   // pés virtuais: cada lado da malha de baixo vai até o seu pé
vec3 sceneAnim(vec3 p, vec2 s){ if(s.x<=0.0) return p; float k = floor(s.y), ph = fract(s.y)*6.2831853;
  if(k<1.5){ float a = uT*1.6+ph+p.x*1.3+p.z*0.9; p.x += sin(a)*s.x; p.z += sin(a*0.8+1.7)*s.x*0.6; }
  else if(k<2.5){ float a = uT*5.5+ph+(p.x+p.z)*5.0; p.x += sin(a)*s.x*0.6; p.z += cos(a*0.9)*s.x; p.y += sin(a*1.3)*s.x*0.3; }
  else if(k<3.5){ p.y += (sin(p.x*1.2+uT*1.9)+0.7*sin(p.z*1.6-uT*1.4))*s.x; }
  else if(k<4.5){ p.y += sin(uT*1.3+ph)*s.x; p.x += sin(uT*0.9+ph)*s.x*0.25; }
  else if(k<5.5){ float w = max(sin(uT*2.2-ph),0.0); p.y += w*w*0.07+abs(sin(uT*7.0+ph*5.0))*0.012; }
  else if(k<7.5){ float a = uT*0.8+ph; p.x += sin(a)*s.x; p.z += sin(a*2.0)*s.x*0.5; p.y += sin(a*2.3)*0.17; }
  return p; }
vec3 swayed(vec3 p, vec2 s){ if(uScene==1) return sceneAnim(p,s); if(uBendOn==1) return mix(p + footShift(p), (uBend*vec4(p,1.0)).xyz, s.x); if(s.x<=0.0) return p;
  float a = atan(p.z,p.x), f = (a+3.14159265)/6.2831853*10.0-0.5, fl = floor(f); vec3 q = mix(uSect[int(mod(fl,10.0))], uSect[int(mod(fl+1.0,10.0))], f-fl)*s.x; q.y *= 0.4;
  vec2 r = vec2(cos(a),sin(a)); q.xz += r*sin(uT*2.2+a*4.0+s.x*3.0)*0.006*s.x; float inw = dot(q.xz,r); if(inw<0.0) q.xz -= r*inw*0.75; q.y += abs(inw)*0.15*s.x; return p+q; }
uniform int uDripMode; uniform vec4 uDrip[12]; uniform vec3 uDripK;
float dripD(vec3 pp){ float th = atan(pp.z,pp.x-uDripK.x), rr = length(vec2(pp.x-uDripK.x,pp.z)), h = 0.0;
  for(int i=0;i<12;i++){ vec4 d = uDrip[i]; if(d.z<0.3) continue; float dth = mod(th-d.x+3.14159265,6.2831853)-3.14159265, ds = dth*rr, dy = pp.y-d.y, R = d.z, b = 1.0-(ds*ds+dy*dy*0.55)/(R*R), hb = b>0.0 ? sqrt(b)*R*0.65 : 0.0, ht = 0.0;
    if(dy<0.0 && dy>-d.w){ float k = 1.0+dy/d.w, w = R*0.62*k+0.6, q = 1.0-(ds/w)*(ds/w); if(q>0.0) ht = sqrt(q)*R*0.38*k; } h = max(h,max(hb,ht)); } return h; }
float dripH(vec3 p){ float th = atan(p.z,p.x), v = p.y, cr = sqrt(max(1.0-v*v,0.04)), h = 0.0;
  for(int i=0;i<12;i++){ vec4 d = uDrip[i]; if(d.z<0.02) continue; float dth = mod(th-d.x+3.14159265,6.2831853)-3.14159265, ds = dth*cr, dv = v-d.y, R = d.z, b = 1.0-(ds*ds+dv*dv)/(R*R), hb = b>0.0 ? sqrt(b)*R*0.7 : 0.0, ht = 0.0;
    if(dv>0.0 && dv<d.w){ float k = 1.0-dv/d.w, w = R*0.6*k+0.02, q = 1.0-(ds/w)*(ds/w); if(q>0.0) ht = sqrt(q)*R*0.4*k; } h = max(h,max(hb,ht)); } return h; }
vec3 pxOf(vec3 p){ return vec3(p.x*uDripK.y+uDripK.x, uDripK.z-p.y*uDripK.y, p.z*uDripK.y); }
float dripDisp(vec3 p){ if(uDripMode==0) return 0.0; return uDripMode==3 ? dripH(normalize(p)) : 1.7*dripD(pxOf(p))/uDripK.y; }
// contorno com espessura mínima na tela (uPx: tamanho de um pixel a 1 m): de longe o casco ficava mais fino que um pixel e virava pontilhado
uniform float uPx;
void main(){ float k = uFade==1 ? smoothstep(uFadeA,uFadeB,aP.y) : 1.0; vec3 n = normalize(mix(aN,aN2,uMorph)); vec4 w = uM * vec4(swayed(mix(aP,aP2,uMorph),aS) + n*dripDisp(aP), 1.0);
  if(uW*k>0.0){ vec3 o = mat3(uM)*(n*uW*k); float L = length(o); if(L>0.0) w.xyz += o*(max(L, distance(w.xyz,uEye)*uPx*1.25*k)/L); }
  vHW = w.xyz; vDist = distance(w.xyz,uEye); gl_Position = uVP * vec4(w.xyz + normalize(w.xyz-uEye)*uPush, 1.0); })";
static const char* kHullFS = R"(#version 330 core
in float vDist; in vec3 vHW; out vec4 o; uniform float uHatchR, uT; uniform sampler2D uNoise; float hnoise(vec2 p){ return texture(uNoise,p/64.0).r; }
uniform vec4 uSmA, uSmB; uniform float uSmC;
// rastro de velocidade rasgado como labareda: a partir da frente, o corpo esticado é cortado em línguas de comprimentos desiguais que tremulam
void smearCut(vec3 w){ if(uSmA.w<=0.0) return; vec3 r = w-uSmA.xyz, b = uSmB.xyz; float al = dot(r,b)/uSmA.w; if(uSmC<0.0 && al<0.12) return; if(al<uSmC) discard;
  vec3 a1 = normalize(cross(b, abs(b.y)<0.9 ? vec3(0,1,0) : vec3(1,0,0))), a2 = cross(b,a1); vec2 pp = vec2(dot(r,a1),dot(r,a2))*uSmB.w;
  float n = hnoise(vec2(pp.x*1.9 + sin(al*5.0+uT*9.0)*0.25, pp.y*1.9 + uT*1.1)), len = 0.12 + 0.95*pow(smoothstep(0.3,0.78,n),1.6);
  if(al > len) discard; }
void hatch(float k){ if(k<0.999 && fract((gl_FragCoord.x+gl_FragCoord.y)/7.0) > mix(0.5,1.0,k)) discard; }
void main(){ smearCut(vHW); hatch(smoothstep(uHatchR-0.8,uHatchR,vDist)); o = vec4(0.15,0.09,0.09,1.0); })";
static const char* kDepthVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=4) in vec3 aP2; layout(location=6) in vec2 aS; uniform mat4 uVP, uM; uniform float uMorph, uT;
uniform mat4 uBend; uniform int uBendOn, uScene; uniform vec3 uSect[10];
uniform vec3 uFoot[2]; vec3 footShift(vec3 p){ return mix(uFoot[0], uFoot[1], smoothstep(-0.06,0.06,p.x)); }   // pés virtuais: cada lado da malha de baixo vai até o seu pé
vec3 sceneAnim(vec3 p, vec2 s){ if(s.x<=0.0) return p; float k = floor(s.y), ph = fract(s.y)*6.2831853;
  if(k<1.5){ float a = uT*1.6+ph+p.x*1.3+p.z*0.9; p.x += sin(a)*s.x; p.z += sin(a*0.8+1.7)*s.x*0.6; }
  else if(k<2.5){ float a = uT*5.5+ph+(p.x+p.z)*5.0; p.x += sin(a)*s.x*0.6; p.z += cos(a*0.9)*s.x; p.y += sin(a*1.3)*s.x*0.3; }
  else if(k<3.5){ p.y += (sin(p.x*1.2+uT*1.9)+0.7*sin(p.z*1.6-uT*1.4))*s.x; }
  else if(k<4.5){ p.y += sin(uT*1.3+ph)*s.x; p.x += sin(uT*0.9+ph)*s.x*0.25; }
  else if(k<5.5){ float w = max(sin(uT*2.2-ph),0.0); p.y += w*w*0.07+abs(sin(uT*7.0+ph*5.0))*0.012; }
  else if(k<7.5){ float a = uT*0.8+ph; p.x += sin(a)*s.x; p.z += sin(a*2.0)*s.x*0.5; p.y += sin(a*2.3)*0.17; }
  return p; }
vec3 swayed(vec3 p, vec2 s){ if(uScene==1) return sceneAnim(p,s); if(uBendOn==1) return mix(p + footShift(p), (uBend*vec4(p,1.0)).xyz, s.x); if(s.x<=0.0) return p;
  float a = atan(p.z,p.x), f = (a+3.14159265)/6.2831853*10.0-0.5, fl = floor(f); vec3 q = mix(uSect[int(mod(fl,10.0))], uSect[int(mod(fl+1.0,10.0))], f-fl)*s.x; q.y *= 0.4;
  vec2 r = vec2(cos(a),sin(a)); q.xz += r*sin(uT*2.2+a*4.0+s.x*3.0)*0.006*s.x; float inw = dot(q.xz,r); if(inw<0.0) q.xz -= r*inw*0.75; q.y += abs(inw)*0.15*s.x; return p+q; }
void main(){ gl_Position = uVP * uM * vec4(swayed(mix(aP,aP2,uMorph),aS),1.0); })";
static const char* kDepthFS = R"(#version 330 core
void main(){})";
static const char* kScreenVS = R"(#version 330 core
out vec2 vU; void main(){ vec2 p = vec2((gl_VertexID==1)?3.0:-1.0, (gl_VertexID==2)?3.0:-1.0); vU = p*0.5+0.5; gl_Position = vec4(p,0.0,1.0); })";
static const char* kBrightFS = R"(#version 330 core
in vec2 vU; out vec4 o; uniform sampler2D uTex; void main(){ vec3 c = texture(uTex,vU).rgb; float l = max(c.r,max(c.g,c.b)); o = vec4(c*smoothstep(0.93,1.0,l),1.0); })";
static const char* kBlurFS = R"(#version 330 core
in vec2 vU; out vec4 o; uniform sampler2D uTex; uniform vec2 uDir; void main(){ vec3 c = texture(uTex,vU).rgb*0.227; c += (texture(uTex,vU+uDir*1.38).rgb+texture(uTex,vU-uDir*1.38).rgb)*0.316; c += (texture(uTex,vU+uDir*3.23).rgb+texture(uTex,vU-uDir*3.23).rgb)*0.07; o = vec4(c,1.0); })";
// acabamento: brilho, cor quente e mais viva, e os cantos um pouco escuros
static const char* kPostFS = R"(#version 330 core
in vec2 vU; out vec4 o; uniform sampler2D uTex, uBloom; uniform float uImpact, uChroma; uniform vec4 uShock; uniform vec2 uRes;
// do jogo 2D (ring_renderer.cpp): onda de choque que distorce a imagem a partir do golpe (com relevo: a crista clareia, o vale escurece)
// e a aberração cromática que separa vermelho e azul conforme o tremor
void main(){ vec2 uv = vU; float relief = 0.0;
  if(uShock.z>=0.0 && uShock.z<=1.0 && uShock.w>0.0){ float aspect = uRes.x/uRes.y; vec2 d = uv-uShock.xy; d.x *= aspect; float dist = length(d), R = uShock.z*0.9, w = 0.12;
    if(abs(dist-R)<w){ float fade = 1.0-uShock.z, wave = sin((dist-R)/w*3.14159265); vec2 nd = normalize(vec2(d.x/aspect,d.y)+vec2(1e-6)); uv += nd*wave*uShock.w*fade*0.06; relief = cos((dist-R)/w*3.14159265*0.5)*wave*uShock.w*fade; } }
  vec3 c; if(uChroma>0.0005){ vec2 sh = (uv-0.5)*uChroma; c = vec3(texture(uTex,uv+sh).r, texture(uTex,uv).g, texture(uTex,uv-sh).b); } else c = texture(uTex,uv).rgb;
  c += texture(uBloom,uv).rgb*0.4; c *= 1.0 + relief*0.45;
  c = pow(max(c,vec3(0.0)), vec3(0.97,1.0,1.05)); float l = dot(c,vec3(0.3,0.59,0.11)); c = mix(vec3(l),c,1.1); c = mix(c, c*c*(3.0-2.0*c), 0.16);
  vec2 q = vU-0.5; c *= 1.0-0.42*dot(q,q)*smoothstep(0.1,0.5,length(q))*2.0;
  // quadro de impacto de mangá: +1 invertido (claro vira nanquim), -1 positivo (papel branco com o escuro em nanquim)
  if(uImpact!=0.0){ float l = dot(c,vec3(0.3,0.59,0.11)), b = smoothstep(0.3,0.38,l); c = vec3(uImpact>0.0 ? 1.0-b : b); }
  o = vec4(c,1.0); })";
// peças do golpe: malha 3D por instância. A nuvem é uma esfera com calombos; faísca, lasca e risco são estilhaços
static const char* kPartVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=3) in mat4 iM; layout(location=7) in vec3 iC; layout(location=8) in vec3 iX;
uniform mat4 uVP; uniform float uOut; out vec3 vN, vW, vC, vX;
void main(){ float k = iX.z, s = iX.y; vec3 p = aP, n = aN;
  bool lump = k<0.5 || (k>3.5 && k<4.5) || (k>5.5 && k<6.5) || k>7.5;
  if(lump){ vec3 fq = vec3(4.4,5.1,4.7)*(0.72+0.55*fract(vec3(s*2.3,s*3.1,s*1.3))), a1 = p*fq+vec3(s,s*1.7,-s*0.6), sn = sin(a1); float f = abs(sn.x)+abs(sn.y)+abs(sn.z); vec3 g = sign(sn)*cos(a1)*fq; vec3 N = normalize(aP); p = N*(0.78+0.17*f); n = normalize(N-0.17*(g-dot(g,N)*N)); }
  vec4 w = iM*vec4(p,1.0); vN = normalize(mat3(iM)*n); vW = w.xyz; vC = iC; vX = iX;
  float o = (k<0.5 || (k>3.5 && k<4.5) || k>7.5) ? uOut*min(1.0,length(iM[0].xyz)/0.16) : (k>4.5 || k<2.5) ? 0.0 : uOut*0.45; w.xyz += normalize(mat3(iM)*normalize(aP))*o; gl_Position = uVP*w; })";
// corte duro, como desenho: nuvem em três bandas de luz do sol; fogo em faixas pelo ângulo com a câmera; faísca acesa
static const char* kPartFS = R"(#version 330 core
in vec3 vN, vW, vC, vX; out vec4 o; uniform vec3 uSun, uEye, uTint; uniform int uInk;
void main(){ if(uInk==1){ o = vec4(0.15,0.09,0.09,1.0); return; } float k = vX.z, t = vX.x; vec3 n = normalize(vN), c;
  if(k<0.5){ float d = dot(n,uSun); c = vC*(d>0.1 ? vec3(1.1,1.06,0.98) : d>-0.3 ? vec3(0.86,0.8,0.78) : uTint*0.66); }
  else if(k>3.5 && k<4.5){ float f = dot(n,normalize(uEye-vW)); c = f>0.74 ? vec3(1.0,0.93,0.46) : f>0.42 ? vec3(1.0,0.56,0.1) : vec3(0.88,0.22,0.06); float d = dot(n,uSun); vec3 smoke = vec3(0.56,0.5,0.47)*(d>0.1 ? 1.1 : 0.72); c = mix(c,smoke,step(0.62,t)); }
  else if(k>8.5){ float d = dot(n,uSun); c = vC*(d>0.15 ? vec3(1.0) : d>-0.35 ? vec3(0.84,0.94,0.99) : vec3(0.64,0.84,0.95));   // espuma: branca, azul-clara, azul com retícula
    if(d<=-0.35){ vec2 q = mat2(0.7071,-0.7071,0.7071,0.7071)*gl_FragCoord.xy/6.0, f = fract(q)-0.5; if(length(f)<0.22) c *= 0.88; } }
  else if(k>7.5){ float f = dot(n,normalize(uEye-vW)); c = f>0.74 ? mix(vC,vec3(1.0),0.55) : f>0.4 ? vC : vC*0.62; }   // energia: miolo claro, borda escura
  else if(k>5.5 && k<6.5){ c = vC*1.18; }   // acesa: sem sombra, passa do branco e brilha
  else if(k>4.5 || k<2.5){ c = vC; }
  else { c = vC*(dot(n,uSun)>0.0 ? 1.0 : 0.72); }
  o = vec4(c,1.0); })";
// peças jogadas: cor por vértice ou do atlas (roupas com a arte original), luz em três faixas de quadrinho, verso do pano mais escuro; o contorno é um casco invertido
static const char* kPropVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=2) in vec3 aC; layout(location=3) in vec2 aU; layout(location=4) in float aI;
uniform mat4 uVP; uniform int uHull; out vec3 vW, vN, vC; out vec2 vU;
void main(){ vN = aN; vC = aC; vU = aU; vec3 p = aP; if(uHull==1){ if(aI<=0.0){ gl_Position = vec4(0.0,0.0,2.0,1.0); return; } p += normalize(aN)*aI; } vW = p; gl_Position = uVP*vec4(p,1.0); })";
static const char* kPropFS = R"(#version 330 core
in vec3 vW, vN, vC; in vec2 vU; out vec4 o; uniform vec3 uSun, uEye, uTint; uniform int uHull; uniform sampler2D uTex; uniform float uAlpha;
void main(){ if(uHull==1){ o = vec4(0.06,0.04,0.05,uAlpha); return; }
  if(vU.x < -1.5){ o = vec4(vC, uAlpha); return; }   // aceso (veio de lava): sem luz
  vec3 c = vC; if(vU.x >= 0.0){ vec4 t = texture(uTex, vU); if(t.a < 0.5) discard; c *= t.rgb / t.a; }
  vec3 n = normalize(vN); if(!gl_FrontFacing){ n = -n; c *= 0.7; }
  float d = dot(n, normalize(uSun)), f = dot(n, normalize(uEye - vW));
  c *= d > 0.3 ? vec3(1.05,1.03,0.99) : d > -0.25 ? vec3(0.9,0.87,0.88) : mix(vec3(0.74), uTint*0.7, 0.4);   // sombra neutra: o rosa continua rosa
  if(f > 0.0 && f < 0.22 && vU.x < 0.0) c *= 0.8;
  o = vec4(c, uAlpha); })";
// água da onda em faixas de quadrinho: fundo, piscina e clara pela altura e pela luz; dentro do tubo mais escuro; crista, pé e riscos de espuma
// correndo no sentido da onda; borda do lábio rasgada em respingo; retícula na sombra; brilho de borda e nanquim na silhueta
static const char* kWaterVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=2) in vec3 aA; uniform mat4 uVP; out vec3 vW, vN, vA;
void main(){ vW = aP; vN = aN; vA = aA; gl_Position = uVP*vec4(aP,1.0); })";
static const char* kWaterFS = R"(#version 330 core
in vec3 vW, vN, vA; out vec4 o; uniform vec3 uEye, uSun; uniform float uT, uH, uDots, uFoamy, uLava; uniform sampler2D uNoise;   // uLava: a mesma coluna em lava (gêiser da Valéria)
float noise(vec2 p){ return texture(uNoise,p/64.0).r; }
void main(){ float u = vA.x, s = vA.y; bool lip = vA.z < 0.5; vec3 V = normalize(uEye-vW), n = normalize(vN); float fv = dot(n,V); bool inside = fv < 0.0; if(inside){ n = -n; fv = -fv; }
  if(lip && s > 0.965 - noise(vec2(u*38.0, uT*6.0))*0.08) discard;
  float hgt = clamp(vW.y/max(uH,0.05), 0.0, 1.0), lam = dot(n,uSun);
  const vec3 deepW = vec3(0.05,0.27,0.5), midW = vec3(0.1,0.56,0.76), liteW = vec3(0.45,0.85,0.93), foamW = vec3(0.96,0.99,1.0), inkW = vec3(0.04,0.12,0.22);
  const vec3 deepL = vec3(0.6,0.07,0.03), midL = vec3(0.97,0.34,0.05), liteL = vec3(1.0,0.7,0.14), foamL = vec3(1.0,0.96,0.6), inkL = vec3(0.16,0.03,0.02);
  vec3 deep = mix(deepW,deepL,uLava), mid = mix(midW,midL,uLava), lite = mix(liteW,liteL,uLava), foam = mix(foamW,foamL,uLava), ink = mix(inkW,inkL,uLava);
  float tone = hgt*0.7 + 0.38*smoothstep(-0.3,0.7,lam) - (inside ? 0.4 : 0.0) + (lip ? 0.0 : 0.16);   // a face da frente é água vista contra a luz: mais clara
  vec3 c = tone < 0.4 ? deep : tone < 0.74 ? mid : lite;
  if(tone < 0.4){ vec2 q = mat2(0.7071,-0.7071,0.7071,0.7071)*gl_FragCoord.xy/uDots, f = fract(q)-0.5; if(length(f) < 0.28) c *= 0.75; }
  if(fv < 0.3 && !inside) c = mix(c, lite*1.06, 0.7);
  float sl = lip ? 6.0 : 2.2, st = noise(vec2(u*24.0, s*sl - uT*2.4)) + 0.5*noise(vec2(u*55.0+3.0, s*sl*2.3 - uT*4.0)), line = 1.0 - smoothstep(0.0, 0.035, abs(st - 0.95));   // na face, riscos compridos subindo
  c = mix(c, foam, line * (0.3 + 0.7*hgt));
  if(uLava>0.5){ float cr = noise(vec2(u*9.0+3.0, s*3.0 - uT*1.3)) + 0.5*noise(vec2(u*23.0, s*8.0 - uT*2.2));   // placas de crosta escura subindo com a lava
    if(cr>1.02) c = vec3(0.24,0.07,0.04)*(lam>0.0 ? 1.15 : 0.8); if(abs(cr-1.02)<0.018) c = ink; }
  // crista: espuma branca desde logo depois do topo, com borda rasgada e contorno; mais para o fim do lábio, espuma em sombra
  if(lip){ float edge = 0.6 + (noise(vec2(u*26.0, uT*3.0))-0.5)*0.2 + (noise(vec2(u*70.0, s*9.0))-0.5)*0.08; float band = edge + 0.11 + noise(vec2(u*33.0, uT*2.0))*0.08, tip = 0.9 - noise(vec2(u*29.0, 5.0))*0.07; bool fm = (s > edge && s < band) || s > tip;   /* espuma só na crista e na ponta do lábio */
    if(fm){ c = lam > 0.0 ? foam : vec3(0.78,0.91,0.98); if(abs(s-edge) < 0.012 || abs(s-band) < 0.01 || abs(s-tip) < 0.01) c = ink; } else if(s > edge) c = mix(c, lite, 0.35); }
  if(vW.y < 0.05 + noise(vec2(u*30.0, uT*2.0))*0.12*uH) c = foam;
  if(!inside && hgt > 0.25 && noise(vW.xz*37.0 + vec2(vW.y*23.0, uT*2.0)) > 0.94) c = vec3(1.0);
  // quebrando: manchas de espuma (com sombra e retícula) tomam a água e abrem buracos até a onda sumir
  float fz = noise(vec2(u*16.0, s*5.0 + uT*1.5)) + 0.4*noise(vec2(u*41.0, s*13.0)); if(fz < uFoamy*1.6 - 0.55) discard;
  if(fz < uFoamy*1.6){ c = lam > 0.1 ? foam : lam > -0.4 ? vec3(0.8,0.92,0.98) : vec3(0.6,0.82,0.95); if(lam <= -0.4){ vec2 q = mat2(0.7071,-0.7071,0.7071,0.7071)*gl_FragCoord.xy/uDots, f = fract(q)-0.5; if(length(f) < 0.22) c *= 0.88; } if(fz > uFoamy*1.6 - 0.05) c = ink; }
  if(fv < 0.1) c = ink;
  o = vec4(c,1.0); })";
static const char* kFlatVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=3) in mat4 iM; layout(location=7) in vec4 iC; uniform mat4 uVP; out vec3 vW, vC; flat out float vKeep; void main(){ vec4 w = iM*vec4(aP,1.0); vW = w.xyz; vC = iC.rgb; vKeep = iC.a; gl_Position = uVP*w; })";
static const char* kFlatFS = R"(#version 330 core
in vec3 vW, vC; flat in float vKeep; out vec4 o; uniform vec3 uEye; uniform float uHatchR;
void hatch(float k){ if(k<0.999 && fract((gl_FragCoord.x+gl_FragCoord.y)/7.0) > mix(0.5,1.0,k)) discard; }
void main(){ hatch(min(vKeep, smoothstep(uHatchR-0.8,uHatchR,distance(vW,uEye)))); o = vec4(vC,1.0); })";
// céu do vinil: do creme no horizonte ao azul no alto, calculado pela direção de cada pixel
static const char* kSkyVS = R"(#version 330 core
uniform mat4 uInvVP; out vec3 vD;
void main(){ vec2 p = vec2((gl_VertexID==1)?3.0:-1.0, (gl_VertexID==2)?3.0:-1.0); vec4 f = uInvVP*vec4(p,1.0,1.0), n = uInvVP*vec4(p,-1.0,1.0); vD = f.xyz/f.w - n.xyz/n.w; gl_Position = vec4(p,1.0,1.0); })";
static const char* kSkyFS = R"(#version 330 core
in vec3 vD; out vec4 o; uniform vec3 uH, uZ; void main(){ vec3 d = normalize(vD); float t = smoothstep(-0.02,0.45,d.y); o = vec4(mix(uH, uZ, pow(t,0.8)),1.0); })";
void Renderer::init() {
    prop_.build(kPropVS, kPropFS); toon_.build(kToonVS, kToonFS); hull_.build(kHullVS, kHullFS); flat_.build(kFlatVS, kFlatFS); sky_.build(kSkyVS, kSkyFS); depth_.build(kDepthVS, kDepthFS); bright_.build(kScreenVS, kBrightFS); blur_.build(kScreenVS, kBlurFS); post_.build(kScreenVS, kPostFS); part_.build(kPartVS, kPartFS); water_.build(kWaterVS, kWaterFS);
    { GLuint va, vb, ib; glGenVertexArrays(1, &va); glGenBuffers(1, &vb); glGenBuffers(1, &ib); waterVao_ = va; waterVbo_ = vb; waterIbo_ = ib; glBindVertexArray(va); glBindBuffer(GL_ARRAY_BUFFER, vb); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
      for (int i = 0; i < 3; i++) { glEnableVertexAttribArray(GLuint(i)); glVertexAttribPointer(GLuint(i), 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(size_t(i) * 3 * sizeof(float))); } glBindVertexArray(0); } glGenVertexArrays(1,&emptyVao_);
    sun_ = glm::normalize(sun_);
    // ruído de 64x64 que se repete, com filtro: substitui a conta por pixel
    { std::vector<unsigned char> nz(64 * 64); unsigned int x = 12345u; for (auto& v : nz) { x = x * 1664525u + 1013904223u; v = static_cast<unsigned char>(x >> 24); }
      glGenTextures(1, &noiseTex_); glBindTexture(GL_TEXTURE_2D, noiseTex_); glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 64, 64, 0, GL_RED, GL_UNSIGNED_BYTE, nz.data()); glPixelStorei(GL_UNPACK_ALIGNMENT, 4); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); }
    // sombra: um mapa do cenário, feito uma vez por arena, e um dos lutadores, a cada quadro
    const int shadowSize[2] = {kShadowStatic, kShadowDynamic};
    glGenFramebuffers(2, shFbo_); glGenTextures(2, shTex_);
    for (int i = 0; i < 2; i++) { glBindTexture(GL_TEXTURE_2D, shTex_[i]); glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowSize[i], shadowSize[i], 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glBindFramebuffer(GL_FRAMEBUFFER, shFbo_[i]); glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shTex_[i], 0); glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE); }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); glBindTexture(GL_TEXTURE_2D, 0);
    const glm::mat4 lightView = glm::lookAt(sun_ * 40.f, glm::vec3(0.f), glm::vec3(0, 1, 0));
    lightS_ = glm::ortho(-kShadowStaticR, kShadowStaticR, -kShadowStaticR, kShadowStaticR, 1.f, 90.f) * lightView; lightD_ = glm::ortho(-kShadowDynamicR, kShadowDynamicR, -kShadowDynamicR, kShadowDynamicR, 1.f, 90.f) * lightView;
    // nuvem: icosaedro dividido três vezes (1280 triângulos); estilhaço: octaedro de faces chapadas
    std::vector<float> blob, shard; { const float t = 1.618034f; const glm::vec3 v[12] = {{-1,t,0},{1,t,0},{-1,-t,0},{1,-t,0},{0,-1,t},{0,1,t},{0,-1,-t},{0,1,-t},{t,0,-1},{t,0,1},{-t,0,-1},{-t,0,1}}; const int f[20][3] = {{0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},{1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},{3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}};
        const auto tri = [&](auto&& self, glm::vec3 a, glm::vec3 b, glm::vec3 c, int lvl) -> void { a = glm::normalize(a); b = glm::normalize(b); c = glm::normalize(c); if (lvl == 0) { for (const glm::vec3& q : {a, b, c}) { const float d[6] = {q.x, q.y, q.z, q.x, q.y, q.z}; blob.insert(blob.end(), d, d + 6); } return; } const glm::vec3 ab = a + b, bc = b + c, ca = c + a; self(self, a, ab, ca, lvl - 1); self(self, b, bc, ab, lvl - 1); self(self, c, ca, bc, lvl - 1); self(self, ab, bc, ca, lvl - 1); };
        for (const auto& q : f) tri(tri, v[q[0]], v[q[1]], v[q[2]], 3);
        const glm::vec3 X(1,0,0), Y(0,1,0), Z(0,0,1); const glm::vec3 oc[8][3] = {{X,Y,Z},{Y,-X,Z},{-X,-Y,Z},{-Y,X,Z},{Y,X,-Z},{-X,Y,-Z},{-Y,-X,-Z},{X,-Y,-Z}};
        for (const auto& q : oc) { const glm::vec3 n = glm::normalize(q[0] + q[1] + q[2]); for (const glm::vec3& p : q) { const float d[6] = {p.x, p.y, p.z, n.x, n.y, n.z}; shard.insert(shard.end(), d, d + 6); } } }
    blobVerts_ = GLsizei(blob.size() / 6); shardVerts_ = GLsizei(shard.size() / 6); glGenBuffers(1, &instVbo_); glGenVertexArrays(1, &blobVao_); glGenVertexArrays(1, &shardVao_);
    const auto meshVao = [&](GLuint vao, const std::vector<float>& d) { GLuint b; glBindVertexArray(vao); glGenBuffers(1, &b); glBindBuffer(GL_ARRAY_BUFFER, b); glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(d.size() * sizeof(float)), d.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr); glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, instVbo_); for (int i = 0; i < 4; i++) { glEnableVertexAttribArray(3 + i); glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 22 * sizeof(float), reinterpret_cast<void*>(size_t(i) * 4 * sizeof(float))); glVertexAttribDivisor(3 + i, 1); }
        glEnableVertexAttribArray(7); glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 22 * sizeof(float), reinterpret_cast<void*>(16 * sizeof(float))); glVertexAttribDivisor(7, 1); glEnableVertexAttribArray(8); glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 22 * sizeof(float), reinterpret_cast<void*>(19 * sizeof(float))); glVertexAttribDivisor(8, 1); };
    meshVao(blobVao_, blob); meshVao(shardVao_, shard); glBindVertexArray(0);
    // cubo unitário: os trechos das cordas
    GLuint vbo;
    std::vector<float> c; const float s[6][3][3] = {{{1,0,0},{0,1,0},{0,0,1}},{{-1,0,0},{0,0,1},{0,1,0}},{{0,1,0},{0,0,1},{1,0,0}},{{0,-1,0},{1,0,0},{0,0,1}},{{0,0,1},{1,0,0},{0,1,0}},{{0,0,-1},{0,1,0},{1,0,0}}};
    for (auto& f : s) { auto v=[&](float a,float b){ for(int i=0;i<3;i++) c.push_back(f[0][i]+a*f[1][i]+b*f[2][i]); }; v(-1,-1);v(1,-1);v(1,1);v(-1,-1);v(1,1);v(-1,1); }
    glGenVertexArrays(1,&cubeVao_); glBindVertexArray(cubeVao_); glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo); glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(c.size()*4),c.data(),GL_STATIC_DRAW); glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glGenBuffers(1, &ropeVbo_); glBindBuffer(GL_ARRAY_BUFFER, ropeVbo_); for (int i = 0; i < 5; i++) { glEnableVertexAttribArray(3 + i); glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 20 * sizeof(float), reinterpret_cast<void*>(size_t(i) * 4 * sizeof(float))); glVertexAttribDivisor(3 + i, 1); }
    glBindVertexArray(0);
}
void Renderer::begin(int w, int h, float a, const Fighter& pl, const Fighter& en, float pScale, float eScale, float pTop, float eTop, float shake, uint32_t frame, float pull, const glm::vec3& focus) {
    ensureTargets(w, h); glBindFramebuffer(GL_FRAMEBUFFER, msFbo_);
    glViewport(0,0,w,h); glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glEnable(GL_MULTISAMPLE);
    if (transparent_) glClearColor(0, 0, 0, 0); else glClearColor(0.965f,0.918f,0.824f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    // atrás da jogadora como placeCamera do jogo, um pouco mais para trás: alta o bastante para ver a oponente por cima do corpo, com as cordas de perto fora do quadro.
    // Gira com o ringue e segue a esquiva. Se ela vai para as cordas, passa para dentro do ringue: de trás a corda ficaria na frente dela
    const float e = pull * pull * (3 - 2 * pull), rad = pl.state == FIGHTER_KNOCKDOWN ? pl.idleRadius() : pl.radius, lat = -pl.offsetX * 0.6f * kWorldUnit;
    const glm::mat4 turn = glm::rotate(glm::mat4(1.f), -a, glm::vec3(0,1,0));
    // placeCamera inteiro: recua com o raio do lutador e com o porte dos dois, sobe, e olha mais para baixo. Em 3D o boneco grande é mesmo maior na
    // tela (no 2D o sprite era chapado), então o tamanho do modelo também afasta
    // King Frog e Cremoso ficaram da altura dos outros: a câmera trata os dois como tamanho normal
    const auto big = [](const Fighter& f) { return f.isBigChar() && f.charId != 1 && f.charId != 3; };
    const float back = (rad - 125.f) + (big(pl) ? 65.f : pl.isSmallChar() ? -20.f : 0.f) + (big(en) ? 25.f : en.isSmallChar() ? -15.f : 0.f), up = (big(pl) ? 25.f : pl.isSmallChar() ? -15.f : 0.f) + (big(en) ? 14.f : en.isSmallChar() ? -10.f : 0.f);
    const float sz = std::max(pScale, 1.f) - 1.f, szE = std::max(eScale, 1.f) - 1.f;
    // câmera no ombro: de lado, por cima do ombro direito do nosso lutador, para o oponente não ficar atrás do corpo; a altura mínima faz a
    // linha do olho passar por cima da cabeça dele e chegar ao rosto do oponente
    const float behind = kCamBack + back * kWorldUnit + sz * 0.6f + szE * 0.3f, D = behind - pl.idleRadius() * kWorldUnit - kSpread, R = (pl.idleRadius() + en.idleRadius()) * kWorldUnit + 2.f * kSpread, overHead = pTop + (pTop - eTop * 0.8f) * D / R, camY = std::max(kCamHeight + up * kWorldUnit + sz * 1.1f + szE * 0.4f, overHead);
    const float side = kCamSide + sz * 1.0f;
    const glm::vec3 eye0(turn * glm::vec4(lat - side, camY, -behind, 1)), target0(turn * glm::vec4(lat * 0.5f - side * 0.08f, kCamLookY + eTop * 0.1f + sz * 0.3f + szE * 0.2f, kCamLookZ, 1));
    const glm::vec3 pullT = focus + glm::vec3(0, 0.95f, 0), target = glm::mix(target0, pullT, e), eye = target + (glm::mix(eye0, focus + glm::vec3(turn * glm::vec4(0.9f, 1.25f, 3.0f, 0)), e) - target) * kLens;
    const glm::vec3 jolt(std::sin(frame * 0.55f) * shake * 0.002f, std::cos(frame * 0.7f) * shake * 0.002f, 0);   // screenShake do motor
    eye_ = eye + jolt; view_ = glm::lookAt(eye_, target + jolt, glm::vec3(0,1,0));
    // golpe que derruba: a câmera avança e mira o ponto do impacto; golpe forte: a câmera balança (tranco na direção do golpe e uma inclinação)
    { glm::vec3 e2 = eye, t2 = target; if (punch_ > 0.f) { t2 = glm::mix(target, punchAt_, punch_ * 0.6f); e2 = glm::mix(eye, punchAt_, punch_ * 0.38f); }
      e2 += kick_; t2 += kick_ * 0.45f; const glm::vec3 fwd = glm::normalize(t2 - e2), up = glm::angleAxis(kickRoll_, fwd) * glm::vec3(0, 1, 0);
      if (punch_ > 0.f || kickRoll_ != 0.f || kick_ != glm::vec3(0.f)) { eye_ = e2 + jolt; view_ = glm::lookAt(eye_, t2 + jolt, up); } }
    // conferência por imagem: KORAMA3D_CAM="ox,oy,oz,ax,ay,az" olha de outro ponto
    if (camOn_) { eye_ = camEye_; view_ = glm::lookAt(camEye_, camAt_, glm::vec3(0, 1, 0)); }
    if (const char* cam = std::getenv("KORAMA3D_CAM")) { float v[6]; if (std::sscanf(cam, "%f,%f,%f,%f,%f,%f", v, v+1, v+2, v+3, v+4, v+5) == 6) { eye_ = glm::vec3(v[0],v[1],v[2]); view_ = glm::lookAt(eye_, glm::vec3(v[3],v[4],v[5]), glm::vec3(0,1,0)); } }
    proj_ = glm::perspective(2.0f * kFovHalf, float(w)/float(h), 0.08f, 400.0f);
    { const glm::mat4 m = glm::transpose(proj_ * view_); const glm::vec4 p[6] = {m[3] + m[0], m[3] - m[0], m[3] + m[1], m[3] - m[1], m[3] + m[2], m[3] - m[2]}; for (int i = 0; i < 6; i++) frustum_[i] = p[i] / glm::length(glm::vec3(p[i])); }
    if (!transparent_) { glDepthMask(GL_FALSE); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); sky_.use(); sky_.set("uInvVP", glm::inverse(proj_ * view_)); sky_.set("uH", horizon_); sky_.set("uZ", zenith_); glBindVertexArray(emptyVao_); glDrawArrays(GL_TRIANGLES, 0, 3); glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); }
}
bool Renderer::project(const glm::vec3& world, int w, int h, glm::vec2& out) const {
    const glm::vec4 c = proj_ * view_ * glm::vec4(world, 1);
    if (c.w <= 0.01f) return false;
    out = {(c.x / c.w * 0.5f + 0.5f) * w, (1 - (c.y / c.w * 0.5f + 0.5f)) * h};
    return true;
}
void Renderer::ropes(const RingRopes& r) {
    profMark(3);
    constexpr int kSeg = 28;
    flat_.use(); flat_.set("uVP", proj_*view_); flat_.set("uEye", eye_); flat_.set("uHatchR", kHatchNear); glBindVertexArray(cubeVao_); glBindBuffer(GL_ARRAY_BUFFER, ropeVbo_); glDisable(GL_CULL_FACE);
    const auto pass = [&](float t, const glm::vec3* col) { ropeInst_.clear();
        for (int s = 0; s < 4; s++) { const glm::vec3 mid = (r.point(s, 0, 0.f) + r.point(s, 0, 1.f)) * 0.5f; const glm::vec2 out = glm::normalize(glm::vec2(mid.x, mid.z)); const float k = 1.f - glm::smoothstep(0.f, 0.6f, glm::dot(glm::vec2(eye_.x, eye_.z) - glm::vec2(mid.x, mid.z), out));
            for (int row = 0; row < r.rows; row++) { glm::vec3 a = r.point(s, row, 0); const glm::vec3 C = col ? *col : ropeCol_[row];
                for (int i = 1; i <= kSeg; i++) { const glm::vec3 b = r.point(s, row, float(i) / kSeg), d = b - a; const float L = glm::length(d); glm::mat4 m = glm::translate(glm::mat4(1.f), (a + b) * 0.5f) * glm::mat4_cast(glm::rotation(glm::vec3(0,0,1), d / L)); m = glm::scale(m, glm::vec3(t, t, L * 0.5f + t * 0.3f)); const float* e = glm::value_ptr(m); ropeInst_.insert(ropeInst_.end(), e, e + 16); const float x[4] = {C.r, C.g, C.b, k}; ropeInst_.insert(ropeInst_.end(), x, x + 4); a = b; } } }
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(ropeInst_.size() * sizeof(float)), ropeInst_.data(), GL_STREAM_DRAW); glDrawArraysInstanced(GL_TRIANGLES, 0, 36, GLsizei(ropeInst_.size() / 20)); };
    // contorno sem gravar profundidade, para a cor passar por cima dele
    const glm::vec3 ink(0.15f, 0.09f, 0.09f); glDepthMask(GL_FALSE); pass(0.033f, &ink); glDepthMask(GL_TRUE); pass(0.027f, nullptr);
    glEnable(GL_CULL_FACE); glBindVertexArray(0);
}
void Renderer::setArena(const Model& m, RingRopes& r) {
    tint_ = {m.num("tintR", 0.9f), m.num("tintG", 0.84f), m.num("tintB", 1.1f)};
    horizon_ = {m.num("horizonR", 0.965f), m.num("horizonG", 0.918f), m.num("horizonB", 0.824f)}; zenith_ = {m.num("zenithR", 0.184f), m.num("zenithG", 0.557f), m.num("zenithB", 0.878f)};
    indoor_ = m.num("indoor", 0) > 0.5f; fogRange_ = {m.num("fogNear", 40.f) * 0.5f, m.num("fogFar", 150.f) * 0.8f}; staticDirty_ = true;   // névoa mais perto: o fundo do alvo se dissolve na luz
    const int n = std::min(RingRopes::kMaxRows, int(m.num("ropeN", 0))); if (n > 0) r.rows = n;
    const glm::vec3 kDefault[3] = {{0.949f,0.427f,0.337f},{1.0f,0.894f,0.671f},{0.231f,0.733f,0.698f}};
    for (int i = 0; i < RingRopes::kMaxRows; i++) { const std::string k = std::to_string(i); if (i < n) { r.heights[i] = m.num(("ropeY" + k).c_str()); ropeCol_[i] = {m.num(("ropeR" + k).c_str()), m.num(("ropeG" + k).c_str()), m.num(("ropeB" + k).c_str())}; } else ropeCol_[i] = kDefault[std::min(i, 2)]; }
}
void Renderer::arena(Model& m, float t) {
    rim_ = 0.f; paint_ = 1.f; hatchR_ = kHatchNear; time_ = t;
    for (Node& n : m.nodes) if (n.name.rfind("sceneSpin_", 0) == 0) { float x, y, z, sp; if (std::sscanf(n.name.c_str(), "sceneSpin_%f_%f_%f_%f", &x, &y, &z, &sp) == 4) n.rot = n.rot0 * glm::angleAxis(t * sp, glm::vec3(x, y, z)); }
    // o fundo do vinil foi feito para ser visto olhando para -z; aqui a jogadora olha para +z, então a arena gira meia volta
    m.updateWorld(glm::rotate(glm::mat4(1.f), kRigPi, glm::vec3(0,1,0)));
    scene_ = 1; drawTree(m); scene_ = 0;
}
void Renderer::apply(Model& m, const FighterPose& p) {
    Node& up = m.nodes[m.need("upper")]; up.pos = p.upperPos; up.rot = glm::quat(glm::vec3(p.lean, p.twist, p.roll)); up.scale = p.bodyScale;
    Node& ne = m.nodes[m.need("neck")]; ne.pos = glm::vec3(0, m.NECK - m.WAIST, 0); ne.rot = glm::quat(glm::vec3(-p.lean*0.5f + p.neckTilt, -p.twist*0.4f + p.neckTurn, 0)); ne.scale = p.headScale;
    Node& hi = m.nodes[m.need("hips")]; hi.pos = p.hipsPos; hi.rot = glm::quat(glm::vec3(p.lean*0.45f, p.twist*0.5f, p.roll*0.55f));
    const char* LEG[2] = {"legL","legR"}, *BOOT[2] = {"bootL","bootR"}, *GLV[2] = {"gloveL","gloveR"};
    for (int i=0;i<2;i++) {
        Node& g = m.nodes[m.need(GLV[i])]; g.pos = p.glove[i]; g.rot = p.gloveRot[i]; g.scale = p.gloveScale[i]; m.setMorphUnder(m.need(GLV[i]), p.gloveOpen);
        if (!m.has("\"hasLegs\":true")) continue;
        Node& b = m.nodes[m.need(BOOT[i])]; b.pos = p.footPos[i];
        Node& l = m.nodes[m.need(LEG[i])]; l.pos = p.legPos[i]; const float len = glm::length(p.legDirLen[i]);
        l.rot = glm::rotation(glm::vec3(0,-1,0), p.legDirLen[i]/len); l.scale = {1, len/m.legLen, 1};
    }
    const glm::mat4 base = p.root * glm::scale(glm::mat4(1.f), glm::vec3(m.scale));
    base_ = base; footShift_[0] = p.footShift[0]; footShift_[1] = p.footShift[1];
    bend_ = base * glm::translate(glm::mat4(1.f), hi.pos) * glm::mat4_cast(hi.rot) * glm::translate(glm::mat4(1.f), glm::vec3(0, -m.HIPS, 0)) * glm::inverse(base);   // o quadril em relação ao descanso
    m.updateWorld(base);
}
void Renderer::drawPrim(const Node& n, const Prim& pr, bool hull) {
    // peça espelhada (escala negativa) inverte o sentido das faces
    if (st_.node != &n) { st_.node = &n; const bool sameWorld = st_.world == n.world;
        if (!sameWorld) { st_.world = n.world; const int cw = glm::determinant(glm::mat3(n.world)) < 0; if (cw != st_.cw) { st_.cw = cw; glFrontFace(cw ? GL_CW : GL_CCW); } (hull ? hull_ : toon_).set("uM", n.world); if (!hull) { const glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(n.world))); glUniformMatrix3fv(toon_.loc("uNM"), 1, GL_FALSE, glm::value_ptr(nm)); } }
        const int dm = n.drip; if (dm != st_.drip) { st_.drip = dm; Shader& ds = hull ? hull_ : toon_; ds.set("uDripMode", dm); if (dm) { const DripSet& set = dm == 1 ? dripBody_ : dm == 2 ? dripHead_ : dripHand_; glUniform4fv(ds.loc("uDrip"), DripSet::kMax, &set.u[0].x); ds.set("uDripK", dripK_); } }
        const int bend = n.lower ? 1 : 0; Shader& sh = hull ? hull_ : toon_; if (bend != st_.bend) { st_.bend = bend; sh.set("uBendOn", bend); } if (n.lower) { sh.set("uBend", glm::inverse(n.world) * bend_ * n.world); const glm::mat3 toNode = glm::inverse(glm::mat3(glm::inverse(base_) * n.world)); const glm::vec3 f[2] = {toNode * footShift_[0], toNode * footShift_[1]}; glUniform3fv(sh.loc("uFoot"), 2, &f[0].x); } }
    glBindVertexArray(pr.vao);
    const float mo = pr.morph ? n.morph : 0.f; Shader& sh = hull ? hull_ : toon_; if (mo != st_.morph) { st_.morph = mo; sh.set("uMorph", mo); }
    if (hull) { const float w = n.outlineW * 0.72f; if (w != st_.w) { st_.w = w; hull_.set("uW", w); } const int f = n.fade ? 1 : 0; if (f != st_.fade) { st_.fade = f; hull_.set("uFade", f); } if (f) { hull_.set("uFadeA", n.fadeA); hull_.set("uFadeB", n.fadeB); } }
    else { const glm::vec4 col = (n.tint.a >= 0.f ? glm::vec4(glm::vec3(n.tint), n.tint.a * pr.color.a) : pr.color) * glm::vec4(1, 1, 1, n.ghost); const bool blend = pr.blend || n.ghost < 0.99f || (n.tint.a >= 0.f && col.a < 0.99f);   /* só o que é transparente de fato: material, efeito gravado ou fantasma (várias peças opacas têm alfa 0,98) */ const int sf = scene_ ? pr.surf : 0; if (sf != st_.surf) { st_.surf = sf; toon_.set("uSurf", sf); } if (sf == 1) toon_.set("uGrain", glm::normalize(glm::mat3(n.world) * pr.grain));
        if (pr.pz != st_.pz) { st_.pz = pr.pz; toon_.set("uPzKind", pr.pz); }
        if (pr.texMix > 0.f || st_.mix != 0.f) { st_.mix = pr.texMix; toon_.set("uTexMix", pr.texMix); if (pr.texMix > 0.f) { glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, pr.texB); glActiveTexture(GL_TEXTURE0); } }
        if (col != st_.color) { st_.color = col; toon_.set("uColor", col); } const int ht = pr.tex ? 1 : 0, hb = pr.tex2 ? 1 : 0, vc = pr.vcol ? 1 : 0, ul = pr.unlit ? 1 : 0, two = pr.twoSided ? 1 : 0;
        if (ht != st_.hasTex) { st_.hasTex = ht; toon_.set("uHasTex", ht); } if (hb != st_.hasBack) { st_.hasBack = hb; toon_.set("uHasBack", hb); } if (vc != st_.vcol) { st_.vcol = vc; toon_.set("uVCol", vc); } if (ul != st_.unlit) { st_.unlit = ul; toon_.set("uUnlit", ul); }
        if (pr.tex2 && pr.tex2 != st_.tex2) { st_.tex2 = pr.tex2; glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, pr.tex2); glActiveTexture(GL_TEXTURE0); } if (pr.tex && pr.tex != st_.tex) { st_.tex = pr.tex; glBindTexture(GL_TEXTURE_2D, pr.tex); }
        if (two != st_.two) { st_.two = two; if (two) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE); } if (blend) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); } }
    drawCalls++; tris += pr.count / 3; if (pr.indexed) glDrawElements(GL_TRIANGLES, pr.count, GL_UNSIGNED_INT, nullptr); else glDrawArrays(GL_TRIANGLES, 0, pr.count);
    if (!hull && (pr.blend || n.ghost < 0.99f || (n.tint.a >= 0.f && n.tint.a * pr.color.a * n.ghost < 0.99f))) { glDisable(GL_BLEND); glDepthMask(GL_TRUE); }
}
void Renderer::fighter(Model& m, const FighterPose& p, const glm::vec3& lag, const ClothSim& cloth, float t, const Smear* smear) {
    // peças soltas: serras giram; tranças, placas e rabo são molas que seguem o atraso do corpo e uma brisa
    const bool umb = m.has("\"gloveMode\":\"umb\"");
    if (umb) { const char* GLV[2] = {"gloveL", "gloveR"}; for (int i = 0; i < 2; i++) { const auto go = [&](auto&& self, int id) -> void { Node& n = m.nodes[id]; if (n.name == "spin") n.rot = n.rot0 * glm::angleAxis(p.gloveSpin[i], glm::vec3(0, 0, 1)); for (const int c : n.kids) self(self, c); }; go(go, m.need(GLV[i])); } }
    int k = 0; for (Node& n : m.nodes) { if (n.name == "spin" && !umb) n.rot = n.rot0 * glm::angleAxis(t * 9.f, glm::vec3(0, 1, 0)); else if (n.name == "sway") { const float ph = float(k++) * 0.9f;
        float ax = std::clamp(lag.z * 3.f + 0.05f * std::sin(t * 1.7f + ph), -0.5f, 0.5f); const float az = std::clamp(-lag.x * 3.f + 0.05f * std::sin(t * 1.3f + ph), -0.5f, 0.5f);
        // aba presa na frente ou atrás do corpo (a faixa do cinto da Espinha): só balança para fora, senão a ponta entra na roupa
        // e abre o quanto a perna mais avançada daquele lado pede, para a coxa não atravessar a aba no passo
        const glm::vec3 at = glm::inverse(n.rot0) * n.pos; if (std::fabs(at.z) > 0.06f && std::fabs(at.z) > std::fabs(at.x)) {
            // lado no corpo pelo lugar da aba no modelo (a de trás vem num grupo girado meia volta); "para fora" é o lado de at.z no próprio nó
            // tudo em relação ao quadril: no soco ele inteiro vai para a frente
            const glm::vec3 piv = glm::vec3(glm::inverse(p.root * glm::scale(glm::mat4(1.f), glm::vec3(m.scale))) * n.world[3]) - p.hipsPos; const bool front = piv.z > 0.f;
            // a coxa na altura da ponta da aba (não o pé, que fica bem mais embaixo)
            const float tipY = piv.y + p.hipsPos.y - kFlapLen * 0.8f; float reach = -1.f;
            if (m.has("\"hasLegs\":true")) for (int i = 0; i < 2; i++) { const glm::vec3 a = p.legPos[i], b = a + p.legDirLen[i]; const float k = std::clamp((a.y - tipY) / std::max(1e-3f, a.y - b.y), 0.f, 1.f), z = a.z + (b.z - a.z) * k - p.hipsPos.z; reach = std::max(reach, front ? z : -z); }
            const float open = std::clamp(std::atan2(reach + kFlapLegR - std::fabs(piv.z), kFlapLen), 0.f, 0.6f) + 0.03f;
            ax = at.z > 0.f ? std::min(ax, -open) : std::max(ax, open); }
        n.rot = n.rot0 * glm::quat(glm::vec3(ax, 0, az)); } }
    rim_ = 0.38f; paint_ = 0.f; hatchR_ = 0.f;
    if (m.has("dripCX")) { if (!dripReady_) { dripReady_ = true; dripBody_.initSkin(12, {{503, 553, 147, 393, 8, 10, 1}, {571, 674, 147, 393, 9.5f, 13.5f, 2}}, 11u, t); dripHead_.initSkin(7, {{366, 492, 142, 398, 8, 11, 1}}, 23u, t); dripHand_.initHand(4, 37u, t); }
        dripK_ = {m.num("dripCX"), m.num("dripS"), m.num("dripFoot")}; dripBody_.step(t); dripHead_.step(t); dripHand_.step(t); }
    apply(m, p); sect_ = cloth.o; time_ = t; shade_ = m.shade; shadeY_ = m.shadeY; pz_ = m.poison; pzInv_ = glm::inverse(p.root * glm::scale(glm::mat4(1.f), glm::vec3(m.scale))); drawTree(m);
    if (smear) {
        // luva rápida: uma cópia esticada para trás ao longo do movimento (frente presa na luva, um pouco mais fina), com a cor e o contorno dela
        const char* GLV[2] = {"gloveL", "gloveR"};
        for (int i = 0; i < 2; i++) { const glm::vec3 d = smear->glove[i]; const float L = glm::length(d); const auto gi = m.byName.find(GLV[i]); if (L < 1e-4f || gi == m.byName.end()) continue;
            const glm::vec3 u = d / L, cur(m.nodes[size_t(gi->second)].world[3]), pivot = cur + u * smear->gloveR; const float s = 1.f + std::min(4.5f, L * 4.5f / (2.f * smear->gloveR)), q = 0.95f;
            const glm::mat3 A = glm::mat3(q) + (s - q) * glm::outerProduct(u, u); const glm::mat4 M = glm::translate(glm::mat4(1.f), pivot) * glm::mat4(A) * glm::translate(glm::mat4(1.f), -pivot);
            std::vector<std::pair<Node*, glm::mat4>> keep; const auto walk = [&](auto&& self, int k) -> void { Node& n = m.nodes[size_t(k)]; keep.push_back({&n, n.world}); n.world = M * n.world; for (const int c : n.kids) self(self, c); }; walk(walk, gi->second);
            smA_ = glm::vec4(cur, std::max(1e-3f, 2.f * smear->gloveR * s - smear->gloveR)); smB_ = glm::vec4(-u, 1.f / smear->gloveR); push_ = 1.6f * smear->gloveR;   // cortado em línguas a partir do centro da luva
            drawTree(m, gi->second); smA_ = glm::vec4(0.f); push_ = 0.f; for (auto& [n, w] : keep) n->world = w; }
    }
    sect_ = nullptr; pz_ = 0.f; shade_ = 0.f;
}
void Renderer::water(const std::vector<float>& verts, const std::vector<unsigned>& idx, float H, float foamy, float t, bool lava) {
    if (idx.empty()) return;
    glBindVertexArray(waterVao_); glBindBuffer(GL_ARRAY_BUFFER, waterVbo_); glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(verts.size() * sizeof(float)), verts.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterIbo_); glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(idx.size() * sizeof(unsigned)), idx.data(), GL_STREAM_DRAW);
    water_.use(); water_.set("uVP", proj_ * view_); water_.set("uEye", eye_); water_.set("uSun", sun_); water_.set("uT", t); water_.set("uH", H); water_.set("uFoamy", foamy); water_.set("uLava", lava ? 1.f : 0.f); water_.set("uDots", kDotPx); water_.set("uNoise", 4);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, noiseTex_); glActiveTexture(GL_TEXTURE0);
    glDisable(GL_CULL_FACE); glDisable(GL_BLEND); glDepthMask(GL_TRUE); glDrawElements(GL_TRIANGLES, GLsizei(idx.size()), GL_UNSIGNED_INT, nullptr); glEnable(GL_CULL_FACE); glBindVertexArray(0); st_ = {};
}
unsigned Renderer::texture(const unsigned char* rgba, int w, int h) {
    GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t); glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glBindTexture(GL_TEXTURE_2D, 0); return t;
}
void Renderer::props(const std::vector<float>& verts, const std::vector<unsigned>& idx, unsigned tex, float alpha) {
    if (!propVao_) { GLuint va, vb, ib; glGenVertexArrays(1, &va); glGenBuffers(1, &vb); glGenBuffers(1, &ib); propVao_ = va; propVbo_ = vb; propIbo_ = ib; glBindVertexArray(va); glBindBuffer(GL_ARRAY_BUFFER, vb); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
        const GLsizei st = 12 * sizeof(float); glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, st, (void*)0); glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, st, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, st, (void*)(6 * sizeof(float))); glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, st, (void*)(9 * sizeof(float))); glEnableVertexAttribArray(4); glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, st, (void*)(11 * sizeof(float))); }
    if (idx.empty()) return;
    glBindVertexArray(propVao_); glBindBuffer(GL_ARRAY_BUFFER, propVbo_); glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(verts.size() * sizeof(float)), verts.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, propIbo_); glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(idx.size() * sizeof(unsigned)), idx.data(), GL_STREAM_DRAW);
    prop_.use(); prop_.set("uVP", proj_ * view_); prop_.set("uSun", sun_); prop_.set("uEye", eye_); prop_.set("uTint", tint_); prop_.set("uTex", 0); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex);
    prop_.set("uAlpha", alpha);
    if (alpha < 0.999f) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); glDisable(GL_CULL_FACE); prop_.set("uHull", 0); glDrawElements(GL_TRIANGLES, GLsizei(idx.size()), GL_UNSIGNED_INT, nullptr); glDepthMask(GL_TRUE); glDisable(GL_BLEND); }   // translúcida: sem contorno
    else { glDisable(GL_BLEND); glDepthMask(GL_TRUE); glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); prop_.set("uHull", 1); glDrawElements(GL_TRIANGLES, GLsizei(idx.size()), GL_UNSIGNED_INT, nullptr);
    glCullFace(GL_BACK); glDisable(GL_CULL_FACE); prop_.set("uHull", 0); glDrawElements(GL_TRIANGLES, GLsizei(idx.size()), GL_UNSIGNED_INT, nullptr); }
    glEnable(GL_CULL_FACE); glBindVertexArray(0); st_ = {};
}
void Renderer::glow(const std::vector<float>& verts, const std::vector<unsigned>& idx) {
    if (idx.empty()) return; props({}, {}, 0);   // garante o VAO das props
    glBindVertexArray(propVao_); glBindBuffer(GL_ARRAY_BUFFER, propVbo_); glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(verts.size() * sizeof(float)), verts.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, propIbo_); glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(idx.size() * sizeof(unsigned)), idx.data(), GL_STREAM_DRAW);
    prop_.use(); prop_.set("uVP", proj_ * view_); prop_.set("uHull", 0); prop_.set("uAlpha", 1.f);
    glDisable(GL_CULL_FACE); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE); glDepthMask(GL_FALSE); glDrawElements(GL_TRIANGLES, GLsizei(idx.size()), GL_UNSIGNED_INT, nullptr);
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glEnable(GL_CULL_FACE); glBindVertexArray(0); st_ = {};
}
void Renderer::effects(Model& m, float t) {   // efeitos gravados do vinil: o mundo, a cor e a visibilidade de cada nó já vêm do tocador
    rim_ = 0.2f; paint_ = 0.f; hatchR_ = 0.f; time_ = t; sect_ = nullptr; drawTree(m);
}
void Renderer::drawTree(Model& m, int from) {
    if (sect_) { hull_.use(); glUniform3fv(hull_.loc("uSect"), ClothSim::kN, &sect_[0].x); toon_.use(); glUniform3fv(toon_.loc("uSect"), ClothSim::kN, &sect_[0].x); }
    // a lista da passada: cenário cortado pela câmera e em ordem de distância; lutador na ordem da árvore
    drawList_.clear();
    const auto collect = [&](auto&& self, int i) -> void { const Node& n = m.nodes[i]; if (!n.visible) return;
        for (const Prim& pr : n.prims) { float d = 0.f;
            if (scene_) { const glm::vec3 c(n.world * glm::vec4((pr.lo + pr.hi) * 0.5f, 1)); const float rad = glm::length(pr.hi - pr.lo) * 0.5f + 1.5f; bool out = false; for (const glm::vec4& pl : frustum_) if (glm::dot(glm::vec3(pl), c) + pl.w < -rad) { out = true; break; } if (out) continue; const glm::vec3 e = c - eye_; d = std::max(0.f, glm::length(e) - rad); if (pr.blend) d = 1e7f - d; }
            drawList_.push_back({&n, &pr, d}); }
        for (int k : n.kids) self(self, k); };
    collect(collect, from >= 0 ? from : m.rootNode);
    if (scene_) std::sort(drawList_.begin(), drawList_.end(), [](const DrawItem& a, const DrawItem& b) { return a.dist < b.dist; });
    profMark(scene_ ? 1 : 4); hull_.use(); hull_.set("uScene", scene_); hull_.set("uHatchR", hatchR_); hull_.set("uPx", 2.f * std::tan(kFovHalf) / float(std::max(fbH_, 1))); hull_.set("uEye", eye_); hull_.set("uVP", proj_*view_); hull_.set("uT", time_); hull_.set("uSmA", smA_); hull_.set("uSmB", smB_); hull_.set("uSmC", smC_); hull_.set("uPush", push_); hull_.set("uNoise", 4); glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, noiseTex_); glActiveTexture(GL_TEXTURE0); glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); st_ = {}; for (const DrawItem& it : drawList_) if (it.n->outlineW > 0 && !it.pr->blend && it.n->ghost > 0.99f) drawPrim(*it.n, *it.pr, true);   /* fantasma sem contorno */ glCullFace(GL_BACK); glFrontFace(GL_CCW);
    profMark(scene_ ? 2 : 5); toon_.use(); toon_.set("uScene", scene_); toon_.set("uHatchR", hatchR_); toon_.set("uVP", proj_*view_); toon_.set("uT", time_); toon_.set("uSmA", smA_); toon_.set("uSmB", smB_); toon_.set("uSmC", smC_); toon_.set("uPush", push_); toon_.set("uSun", sun_); toon_.set("uEye", eye_); toon_.set("uTex", 0); toon_.set("uTex2", 1); toon_.set("uTint", tint_); toon_.set("uFog", horizon_); glUniform2f(toon_.loc("uFogR"), fogRange_.x, fogRange_.y); toon_.set("uRim", rim_); toon_.set("uDots", kDotPx); toon_.set("uSurf", 0); toon_.set("uPaint", paint_); toon_.set("uFlash", flash_); toon_.set("uFlashCol", glm::vec3(1.f, 0.62f, 0.25f)); toon_.set("uLS", lightS_); toon_.set("uLD", lightD_); toon_.set("uShS", 2); toon_.set("uShD", 3); toon_.set("uNoise", 4); toon_.set("uTexB", 5); toon_.set("uTexMix", 0.f); glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, noiseTex_); glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, shTex_[0]); glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, shTex_[1]); glActiveTexture(GL_TEXTURE0);
    toon_.set("uPz", pz_); toon_.set("uShade", shade_); toon_.set("uShadeY", shadeY_); if (shade_ > 0.f) toon_.set("uPzInv", pzInv_); toon_.set("uCloudK", scene_ ? 0.f : cloudK_); toon_.set("uCloud", cloud_); toon_.set("uCloudSil", cloudSil_); if (pz_ > 0.f) toon_.set("uPzInv", pzInv_); st_ = {}; for (const DrawItem& it : drawList_) drawPrim(*it.n, *it.pr, false);
    glEnable(GL_CULL_FACE); glFrontFace(GL_CCW); glBindVertexArray(0);
}
void Renderer::ensureTargets(int w, int h) {
    constexpr int ms = 2;   // suavização: 4x custava 3 a 4 ms a mais na Intel
    if (w == fbW_ && h == fbH_) return;
    if (msFbo_) { glDeleteFramebuffers(1, &msFbo_); glDeleteFramebuffers(1, &resFbo_); glDeleteFramebuffers(2, blurFbo_); glDeleteTextures(1, &msColor_); glDeleteTextures(1, &msDepth_); glDeleteTextures(1, &resTex_); glDeleteTextures(2, blurTex_); }
    fbW_ = w; fbH_ = h;
    glGenFramebuffers(1, &msFbo_); glBindFramebuffer(GL_FRAMEBUFFER, msFbo_); glGenTextures(1, &msColor_); glGenTextures(1, &msDepth_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msColor_); glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, ms, GL_RGBA8, w, h, GL_TRUE); glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msColor_, 0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msDepth_); glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, ms, GL_DEPTH_COMPONENT24, w, h, GL_TRUE); glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, msDepth_, 0);
    const auto color = [](GLuint& fbo, GLuint& tex, int cw, int ch) { glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo); glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cw, ch, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0); };
    color(resFbo_, resTex_, w, h); for (int i = 0; i < 2; i++) color(blurFbo_[i], blurTex_[i], w / 4, h / 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); glBindTexture(GL_TEXTURE_2D, 0); glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}
void Renderer::drawDepth(Model& m) {
    const auto go = [&](auto&& self, int i) -> void { const Node& n = m.nodes[i]; if (!n.visible) return;
        for (const Prim& pr : n.prims) { if (pr.blend || pr.hi.y > depthMaxY_) continue; glBindVertexArray(pr.vao); depth_.set("uM", n.world); depth_.set("uMorph", pr.morph ? n.morph : 0.f); depth_.set("uBendOn", n.lower ? 1 : 0); if (n.lower) depth_.set("uBend", glm::inverse(n.world) * bend_ * n.world); if (pr.indexed) glDrawElements(GL_TRIANGLES, pr.count, GL_UNSIGNED_INT, nullptr); else glDrawArrays(GL_TRIANGLES, 0, pr.count); }
        for (int k : n.kids) self(self, k); };
    go(go, m.rootNode);
}
void Renderer::shadowPass(Model& arena, Model& a, const FighterPose& pa, const ClothSim& clothA, float tA, Model& b, const FighterPose& pb, const ClothSim& clothB, float tB) {
    profMark(0); glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glDisable(GL_CULL_FACE); glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.6f, 3.f); depth_.use();
    const auto begin = [&](int i, const glm::mat4& vp, int size) { glBindFramebuffer(GL_FRAMEBUFFER, shFbo_[i]); glViewport(0, 0, size, size); glClear(GL_DEPTH_BUFFER_BIT); depth_.set("uVP", vp); };
    if (staticDirty_) { begin(0, lightS_, kShadowStatic); depth_.set("uT", 0.f); depth_.set("uScene", 1); arena.updateWorld(glm::rotate(glm::mat4(1.f), kRigPi, glm::vec3(0,1,0))); depthMaxY_ = indoor_ ? 5.f : 1e9f; drawDepth(arena); depthMaxY_ = 1e9f; staticDirty_ = false; }
    begin(1, lightD_, kShadowDynamic); depth_.set("uScene", 0);
    glUniform3fv(depth_.loc("uSect"), ClothSim::kN, &clothA.o[0].x); depth_.set("uT", tA); apply(a, pa); drawDepth(a);
    glUniform3fv(depth_.loc("uSect"), ClothSim::kN, &clothB.o[0].x); depth_.set("uT", tB); apply(b, pb); drawDepth(b);
    glDisable(GL_POLYGON_OFFSET_FILL); glEnable(GL_CULL_FACE); glBindVertexArray(0); glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Renderer::particles(const std::vector<Particle>& list) {
    profMark(6);
    if (list.empty()) return;
    std::vector<float> blobs, shards; blobs.reserve(list.size() * 22); shards.reserve(list.size() * 22);
    for (const Particle& q : list) { const float t = q.age / q.life; const glm::vec3 col = glm::vec3(glm::mix(q.c0, q.c1, t)); glm::mat4 m(1.f); const bool blob = q.kind == kPuff || q.kind == kFire || q.kind == kGlow || q.kind == kEnergy || q.kind == kFoam;
        if (q.kind == kSpark) { const int segs = int(std::ceil(float(q.trailN) * (1.f - 0.65f * t * t))); glm::vec3 a = q.p; const glm::vec3 H(1.f, 0.96f, 0.62f), M(1.f, 0.6f, 0.1f), Tl(0.93f, 0.22f, 0.05f);
            for (int i = 0; i < segs; i++) { const glm::vec3 b = q.trail[i], dd = a - b; const float L = glm::length(dd); if (L > 1e-4f) { const float u = float(i) / float(std::max(1, segs)), w = q.size * (1.f - u) * (1.f - t * t); const glm::vec3 d = dd / L; glm::vec3 up = std::fabs(d.y) > 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0); const glm::vec3 side = glm::normalize(glm::cross(d, up)); up = glm::cross(side, d); const glm::mat4 seg(glm::vec4(d * L * 1.25f, 0), glm::vec4(up * w, 0), glm::vec4(side * w, 0), glm::vec4((a + b) * 0.5f, 1)); const glm::vec3 c = u < 0.3f ? H : u < 0.65f ? M : Tl; const float* e = glm::value_ptr(seg); shards.insert(shards.end(), e, e + 16); const float x[6] = {c.r, c.g, c.b, t, q.seed, float(kSpark)}; shards.insert(shards.end(), x, x + 6); } a = b; }
            continue; }
        if (blob) { const float s = q.hold ? q.size * std::min(1.f, q.age / 4.f) * (t > 0.9f ? (1.f - t) * 10.f : 1.f) : q.size * std::min(1.f, t / 0.14f) * (1.f - t * t * t); m = glm::translate(m, q.p) * glm::rotate(glm::mat4(1.f), q.seed + q.age * 0.012f, glm::normalize(glm::vec3(std::sin(q.seed), 0.6f, std::cos(q.seed)))); m = glm::scale(m, glm::vec3(s * (1.f + 0.24f * std::sin(q.seed * 2.1f)), s * (0.8f + 0.24f * std::sin(q.seed * 3.3f)), s * (1.f - 0.2f * std::cos(q.seed * 1.7f)))); }
        else if (q.kind == kFlake) { m = glm::translate(m, q.p) * glm::rotate(glm::mat4(1.f), q.seed + q.age * 0.28f, glm::normalize(glm::vec3(std::cos(q.seed), 0.5f, std::sin(q.seed)))); m = glm::scale(m, glm::vec3(q.size * 1.3f, q.size * 1.3f, q.size * 0.4f) * (1.f - t * t)); }
        else { const bool fixedLen = q.len > 0.f; const float L = fixedLen ? 0.f : glm::length(q.v); glm::vec3 d = fixedLen ? glm::normalize(q.dir + glm::vec3(0, 1e-5f, 0)) : L > 1e-4f ? q.v / L : glm::vec3(0, 1, 0); glm::vec3 up = std::fabs(d.y) > 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0); const glm::vec3 side = glm::normalize(glm::cross(d, up)); up = glm::cross(side, d); const float w = q.hold ? q.size * 0.42f * std::min(1.f, q.age / 3.f) * (t > 0.8f ? (1.f - t) * 5.f : 1.f) : q.size * 0.42f * (1.f - t * t), len = fixedLen ? q.len : q.size + L * (q.kind == kLine ? 1.6f : 1.4f); m = glm::mat4(glm::vec4(d * len, 0), glm::vec4(up * w, 0), glm::vec4(side * w, 0), glm::vec4(q.p, 1)); }
        auto& out = blob ? blobs : shards; const float* e = glm::value_ptr(m); out.insert(out.end(), e, e + 16); const float x[6] = {col.r, col.g, col.b, t, q.seed, float(q.kind)}; out.insert(out.end(), x, x + 6); }
    part_.use(); part_.set("uVP", proj_ * view_); part_.set("uSun", sun_); part_.set("uEye", eye_); part_.set("uTint", tint_); glEnable(GL_CULL_FACE); glBindBuffer(GL_ARRAY_BUFFER, instVbo_);
    const auto drawAll = [&](GLuint vao, GLsizei verts, const std::vector<float>& d) { if (d.empty()) return; glBindBuffer(GL_ARRAY_BUFFER, instVbo_); glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(d.size() * sizeof(float)), d.data(), GL_STREAM_DRAW); glBindVertexArray(vao); const GLsizei n = GLsizei(d.size() / 22);
        glCullFace(GL_FRONT); part_.set("uInk", 1); part_.set("uOut", 0.016f); glDrawArraysInstanced(GL_TRIANGLES, 0, verts, n); glCullFace(GL_BACK); part_.set("uInk", 0); part_.set("uOut", 0.f); glDrawArraysInstanced(GL_TRIANGLES, 0, verts, n); };
    glFrontFace(GL_CCW); drawAll(blobVao_, blobVerts_, blobs); drawAll(shardVao_, shardVerts_, shards); glBindVertexArray(0);
}
void Renderer::finishToTexture(unsigned tex, int w, int h) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msFbo_); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resFbo_); glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, resFbo_); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex); glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
    glBindTexture(GL_TEXTURE_2D, 0); glBindFramebuffer(GL_FRAMEBUFFER, 0); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glBindVertexArray(0); glUseProgram(0);
}
void Renderer::finish(int w, int h) {
    profMark(7);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msFbo_); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resFbo_); glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_BLEND); glBindVertexArray(emptyVao_); glActiveTexture(GL_TEXTURE0);
    const auto pass = [&](const Shader& sh, GLuint fbo, GLuint src, int pw, int ph) { glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, pw, ph); sh.use(); sh.set("uTex", 0); glBindTexture(GL_TEXTURE_2D, src); };
    const int bw = w / 4, bh = h / 4;
    pass(bright_, blurFbo_[0], resTex_, bw, bh); glDrawArrays(GL_TRIANGLES, 0, 3);
    for (int i = 0; i < 2; i++) { pass(blur_, blurFbo_[1], blurTex_[0], bw, bh); glUniform2f(blur_.loc("uDir"), 1.f / float(bw), 0.f); glDrawArrays(GL_TRIANGLES, 0, 3); pass(blur_, blurFbo_[0], blurTex_[1], bw, bh); glUniform2f(blur_.loc("uDir"), 0.f, 1.f / float(bh)); glDrawArrays(GL_TRIANGLES, 0, 3); }
    pass(post_, 0, resTex_, w, h); post_.set("uBloom", 1); post_.set("uImpact", impact_); post_.set("uChroma", chroma_); post_.set("uShock", shock_); glUniform2f(post_.loc("uRes"), float(w), float(h)); glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, blurTex_[0]); glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTexture(GL_TEXTURE_2D, 0); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0); glBindVertexArray(0); glUseProgram(0);
}
}
