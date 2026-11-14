(window.KR=window.KR||{})["joana/abismo"]=(function(){
// Same five sectors and relative head projection as reactions.js in the game.
// Costumes retain their approved vector artwork, not the old base costume.
const angleArtCache=new Map();
function angleNativeSprites(o){
 if(skillSkinIndex!==0)return null;
 if(skillCharacter==='jack')return o.combat.rx;
 if(skillCharacter==='cremoso'&&(!o.mode||o.mode==='normal')||skillCharacter==='vivaldi'&&(!o.season||o.season==='concert')||skillCharacter==='lady'&&(!o.state||o.state==='normal'))return o.combat.nativeRotation;
 return null;
}
function angleHeadProjection(body,turn,cx,halfWidth){
 if(!turn)return body;
 const sx=1-.32*turn,dx=.16*halfWidth*turn;
 // Only assembly roots are transformed. Their descendants keep local details
 // and receive no second transform (flowers, fringe, glasses, crown, etc.).
 const re=/<g\b[^>]*>|<\/g>/g;let m,depth=0,ownedDepth=-1,out='',at=0;
 while((m=re.exec(body))){
  const tag=m[0],close=tag.startsWith('</');
  out+=body.slice(at,m.index);at=re.lastIndex;
  if(close){if(depth===ownedDepth){out+='</g>';ownedDepth=-1;}out+=tag;depth--;continue;}
  depth++;
  const part=tag.match(/data-(?:joana|marcov|valeria|vivaldi|cremoso|lady)-part="([^"]+)"/)?.[1];
  const head=/data-angle-owner="head"/.test(tag)||/class="[^"]*\banim-head\b/.test(tag)||['head','hair','wig','hat','bandana','stalk','eyes','mouth'].includes(part);
  out+=tag;
  if(head&&ownedDepth<0){ownedDepth=depth;out+=`<g data-native-profile-head="${turn}" transform="translate(${spN(cx+dx)} 0) scale(${spN(sx)} 1) translate(${-cx} 0)">`;}
 }
 return out+body.slice(at);
}
function angleArt(o,frame){
 const cfg=spConfig(o),sprites=angleNativeSprites(o),boss=skillCharacter==='batalhator';
 const key=[o.view,o.state,o.mode,o.season,o.phase,o.shield,o.cone,frame].join('/');
 if(angleArtCache.has(key))return angleArtCache.get(key);
 const view=boss?o.view:frame>=3?'back':'front';
 const parts=skillBase({...o,action:'idle',view,time:0,expression:'game_over',exportSpecialParts:true,exportTurn:0});
 let body=parts.body,origin=boss?'approved-native-camera-view':'approved-profile-adaptation';
 if(sprites){
  // Atlas sprites are square rasters of a centered, aspect-preserving SVG.
  // Undo that square padding instead of stretching it into a tall art box.
  const box=o.combat.nativeRotationBox||[0,0,120,120],side=Math.max(box[2],box[3]),x=box[0]+box[2]/2-side/2,y=box[1]+box[3]/2-side/2;
  body=`<image data-native-angle="RxSpin_${frame}" data-native-angle-box="${box.join(' ')}" href="${sprites['RxSpin_'+frame]}" x="${x}" y="${y}" width="${side}" height="${side}" preserveAspectRatio="none"/>`;origin='native-atlas-sprite';
 }
 else if(!boss){
  const turn=[0,.6,1,.6,0][frame],bodySx=[1,.82,.55,.82,1][frame],hw={jack:40,frog:35,joana:30,marcov:35,valeria:30,vivaldi:40,cremoso:27,lady:50}[skillCharacter],cx=cfg.w/2;
  body=angleHeadProjection(body,turn,cx,hw);
  body=`<g data-native-profile-sector="${frame}" data-native-profile-width="${bodySx}" data-profile-source="reactions.js bodySx and head projection" transform="translate(${cx} 0) scale(${bodySx} 1) translate(${-cx} 0)">${body}</g>`;
 }
 const art={body,hands:parts.hands,defs:parts.defs||'',origin};
 angleArtCache.set(key,art);if(angleArtCache.size>80)angleArtCache.delete(angleArtCache.keys().next().value);
 return art;
}
function angleSurface(body,o,origin){
 // Native atlas pixels already contain their original finish. Adapted vector
 // bodies keep the same low-rate outline noise as their approved idle art;
 // apply it at the current instant, not at the first cached angle's instant.
 if(origin==='native-atlas-sprite'||!['vivaldi','cremoso'].includes(skillCharacter))return body;
 return spInner(skillBase({...o,exportSurface:true,surfaceSvg:'<svg xmlns="http://www.w3.org/2000/svg">'+body+'</svg>'}));
}
function angleStudy(o){
 const cfg=spConfig(o),p=(Math.max(0,o.time||0)%2.4)/2.4,a=p*Math.PI*4+(o.view==='back'?Math.PI:0),sector=Math.floor(a/(Math.PI/4)+.5)%8,frame=[0,1,2,3,4,3,2,1][sector],art=angleArt(o,frame),cx=cfg.w/2,mirror=skillCharacter==='batalhator'?1:sector>4?-1:1;
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${-.4*cfg.w} ${-.4*cfg.h} ${1.8*cfg.w} ${1.8*cfg.h}" data-combat="spin" data-sector="${sector}" data-angle-frame="${frame}" data-angle-art="${art.origin}" data-study="native-five-angle-projection">${art.defs}<g transform="translate(${cx} 0) scale(${mirror} 1) translate(${-cx} 0)">${angleSurface(art.body,o,art.origin)}${o.armed?art.hands.join(''):''}</g></svg>`;
}

const skillBase=(()=>{const makeArt=()=>{// Skin 2: Almirante do Abismo. Same 120×120 Joana frame and side-facing claw.
// Time is explicit so the live laboratory and exported animation use one rig.
const JOANA_ABYSS_PERIOD=6;
function joanaAbyssDreadFrame(view='front',time=0) {
  const back=view==='back';
  const roots=back?[[44,25],[50,25],[56,25],[62,25],[68,25],[74,25],[78,27]]:[[41,25],[45,28],[42,31],[75,26],[77,30]];
  const tips=back?[[29,56],[40,65],[52,60],[62,65],[76,59],[87,62],[94,48]]:[[29,56],[39,67],[24,46],[90,56],[82,66]];
  const f=n=>Number(n.toFixed(3));
  return roots.map(([x,y],i)=>{
    const [tx,ty]=tips[i],side=tx<x?-1:1;
    const phase=time/JOANA_ABYSS_PERIOD*Math.PI*2+i*.81;
    const wave=Math.sin(phase),follow=Math.sin(phase-.8);
    const endX=tx+wave*3.2,endY=ty+follow*2;
    const bendX=(x+tx)/2+side*4+follow*2.6;
    const bendY=(y+ty)/2+5+wave*1.4;
    const d=`M ${x} ${y} C ${f(x+side*6)} ${y+8} ${f(bendX)} ${f(bendY-8)} ${f(bendX)} ${f(bendY)} S ${f(endX-side*8)} ${f(endY+4)} ${f(endX)} ${f(endY)}`;
    const branch=`M ${f(endX-side*2)} ${f(endY)} Q ${f(endX+side*3)} ${f(endY-1)} ${f(endX+side*3.5)} ${f(endY-5)} M ${f(endX)} ${f(endY)} Q ${f(endX+side*5)} ${f(endY+2)} ${f(endX+side*7)} ${f(endY-1)}`;
    return {id:i,d,branch,color:i%2?'#146e73':'#0d9488'};
  });
}

function joanaAbyssDreads({view='front',time=0}={}) {
  return `<g data-part="floating-algae-dreadlocks" fill="none" stroke-linecap="round" stroke-linejoin="round">${joanaAbyssDreadFrame(view,time).map(s=>`
    <g data-strand="${s.id}">
      <path data-dread="${s.id}" data-layer="outline" d="${s.d}" stroke="#061b28" stroke-width="7"/>
      <path data-dread="${s.id}" data-layer="core" d="${s.d}" stroke="${s.color}" stroke-width="4.6"/>
      <path data-dread="${s.id}" data-layer="light" d="${s.d}" stroke="#76f2d7" stroke-width="1" stroke-dasharray="2 7" opacity=".8"/>
      <path data-dread="${s.id}" data-layer="branch" d="${s.branch}" stroke="#4fcbb5" stroke-width="1.1"/>
    </g>`).join('')}</g>`;
}

function joanaAbyssHands({view='front',hand='both'}={}) {
  const back=view==='back';
  const hook=`<g data-part="black-coral-hook" stroke="#050c19" stroke-linejoin="round">
    <path d="M 29 57 L 33 63 L 28 69 L 22 66 L 21 60 Z" fill="#6b8d9e" stroke-width="1.7"/>
    <path d="M 24 65 V 76 C 24 85 36 84 36 75 V 69" fill="none" stroke="#9ce9da" stroke-width="6.4" stroke-linecap="round"/>
    <path d="M 24 65 V 76 C 24 85 36 84 36 75 V 69" fill="none" stroke="#263c4c" stroke-width="4.3" stroke-linecap="round"/>
    <path d="M 36 62 L 30 71 L 36 68 L 40 71 Z" fill="#263c4c" stroke="#9ce9da" stroke-width="1.2"/>
    <path d="M 22 70 L 18 73 L 23 74 M 26 80 L 26 86 L 30 81" fill="#263c4c" stroke="#9ce9da" stroke-width="1.2"/>
    <path d="M 23 68 V 75 Q 23 80 27 81 M 35 68 L 33 72" fill="none" stroke="#e0fff1" stroke-width="1.2"/>
    <path d="M 23 61 L 29 64 M 22 64 L 28 67" stroke="#b2ddd6" stroke-width="1.1"/>
  </g>`;
  const claw=`<g data-part="glass-blue-lobster-claw" data-perspective="standard-joana-side" stroke="#050c19" stroke-linejoin="round">
    <path d="M 79 48 Q 87 47 90 55 L 95 65 L 86 72 L 79 61 Z" fill="#17398c" stroke-width="1.8"/>
    <path d="M 82 50 L 86 54 L 89 64 L 85 66 Z" fill="#2563b5" stroke-width="1"/>
    <g transform="translate(94 66) scale(${back?'-.49':'.49'} .49)" stroke-width="2.6">
      <path d="M -10 10 H 10 V 22 H -10 Z" fill="#334155"/>
      <path d="M -22 -10 C -18 -22 -8 -28 4 -26 C 16 -24 22 -14 22 -2 C 22 10 16 18 4 20 C -8 22 -18 14 -22 4 Z" fill="${back?'#17398c':'#1d4ed8'}"/>
      <path d="M -20 -9 L -8 -22 L 7 -20 L 16 -11 L 10 2 L -3 8 L -17 3 Z" fill="${back?'#2054ae':'#3479dc'}" stroke-width="1"/>
      <path d="M -14 -15 L -5 -20 L -8 -9 L -16 -3 Z M 7 -15 L 12 -10 L 8 -4 Z" fill="#5bbdc8" stroke="none"/>
      <path d="M -16 11 Q -1 23 15 11" fill="none" stroke="#102d68" stroke-width="3"/>
      <g transform="rotate(13 -2 -14)">
        <path d="M -2 -14 C 4 -26 13 -36 24 -36 L 29 -32 C 27 -23 19 -14 10 -8 Z" fill="#1d4ed8"/>
        <path d="M 20 -34 L 29 -32 L 23 -27 Z" fill="#6ae0d1" stroke="none"/>
        <path d="M 8 -20 L 15 -23 L 12 -16 Z M 15 -23 L 22 -26 L 19 -19 Z" fill="#c6f6e8" stroke-width="1.3"/>
      </g>
      <g transform="rotate(-15 -5 4)">
        <path d="M -5 4 C 5 2 16 6 25 14 L 19 23 C 9 20 1 15 -5 10 Z" fill="#102d68"/>
        <path d="M 18 12 L 26 14 L 21 20 Z" fill="#3a79bd" stroke-width="1.3"/>
        <path d="M 4 5 L 11 3 L 8 10 Z" fill="#c6f6e8" stroke-width="1.3"/>
      </g>
      <circle cx="-14" cy="-4" r="3.4" fill="#2dd4bf" stroke-width="1.4"/><circle cx="-14" cy="-4" r="1.1" fill="#d1fff1" stroke="none"/>
      <path d="M -7 12 L -2 14 M 3 12 L 7 10" stroke="#5eead4" stroke-width="1.6" stroke-linecap="round"/>
    </g>
  </g>`;
  return hand==='hook'?hook:hand==='claw'?claw:hook+claw;
}

function joanaAbyssTentacleFrame(view='front',time=0) {
  const back=view==='back',f=n=>Number(n.toFixed(3));
  const strands=[
    [[89,70],[85,77],[88,87],[96,86],[101,85],[96,78],[93,82]],
    [[99,71],[107,76],[116,82],[111,88],[105,93],[100,87],[105,84]],
    [[101,58],[109,50],[116,54],[114,60],[112,65],[108,63],[109,59]]
  ];
  const point=(p0,p1,p2,p3,t)=>[0,1].map(axis=>(1-t)**3*p0[axis]+3*(1-t)**2*t*p1[axis]+3*(1-t)*t*t*p2[axis]+t**3*p3[axis]);
  return strands.map((points,id)=>{
    const phase=time/JOANA_ABYSS_PERIOD*Math.PI*2+id*1.7;
    const moved=points.map(([x,y],index)=>index===0?[x,y]:[x+Math.sin(phase-index*.3)*1.3*index/6,y+Math.cos(phase-index*.4)*1.5*index/6]);
    const p=moved.map(([x,y])=>[f(back?188-x:x),f(y)]);
    const d=`M ${p[0].join(' ')} C ${p.slice(1,4).map(v=>v.join(' ')).join(' ')} C ${p.slice(4).map(v=>v.join(' ')).join(' ')}`;
    const suckers=[.25,.48,.72,.94].map(t=>{const [x,y]=point(...p.slice(0,4),t);return {x:f(x),y:f(y)};});
    return {id,d,suckers};
  });
}

function joanaAbyssMonsterHands({view='front',hand='both',time=0}={}) {
  if(hand==='hook') return joanaAbyssHands({view,hand:'hook'});
  const back=view==='back';
  const tentacles=`<g data-part="abyssal-tentacles" fill="none" stroke-linecap="round" stroke-linejoin="round">${joanaAbyssTentacleFrame(view,time).map(s=>`
    <path data-tentacle="${s.id}" data-layer="outline" d="${s.d}" stroke="#050c19" stroke-width="5.6"/>
    <path data-tentacle="${s.id}" data-layer="core" d="${s.d}" stroke="#166981" stroke-width="3.6"/>
    <path data-tentacle="${s.id}" data-layer="light" d="${s.d}" stroke="#43cbb9" stroke-width=".8" stroke-dasharray="2 6"/>
    ${s.suckers.map((cup,i)=>`<circle data-sucker="${s.id}" data-cup="${i}" cx="${cup.x}" cy="${cup.y}" r="1.05" fill="#cffff0" stroke="#276f73" stroke-width=".55"/><circle data-sucker="${s.id}" data-cup="${i}" cx="${cup.x}" cy="${cup.y}" r=".32" fill="#167879" stroke="none"/>`).join('')}`).join('')}</g>`;
  const claw=`<g data-part="abyssal-maw-claw" data-perspective="standard-joana-side" stroke="#050c19" stroke-linejoin="round">
    <path d="M 79 48 Q 87 47 91 56 L 97 66 L 86 75 L 79 61 Z" fill="#17398c" stroke-width="1.8"/>
    <path d="M 83 51 L 87 56 L 90 65 L 85 69 Z" fill="#2563b5" stroke-width="1"/>
    ${tentacles}
    <g data-part="abyssal-mandible-mass" transform="translate(94 66) scale(${back?'-.58':'.58'} .58)" stroke-width="2.5">
      <path d="M -23 -11 L -18 -24 L -5 -30 L 10 -25 L 20 -14 L 23 0 L 17 15 L 3 23 L -13 19 L -24 6 Z" fill="${back?'#102d68':'#17398c'}"/>
      <path d="M -21 -13 L -11 -24 L -2 -27 L 0 -17 L -10 -9 Z M -20 5 L -10 9 L -6 18 L -15 14 Z" fill="#2769b5" stroke-width="1"/>
      <path d="M -14 -9 Q -4 -21 10 -13 L 16 -3 L 10 10 Q -5 18 -15 7 Z" fill="#071722" stroke="#278f97" stroke-width="1.8"/>
      <path d="M -12 -10 L -9 -2 L -4 -14 L 0 -5 L 5 -15 L 9 -6 L 13 -7 M -12 7 L -8 0 L -5 12 L 0 4 L 3 12 L 8 3 L 11 8" fill="#b8f5e5" stroke-width="1.2"/>
      <g data-part="abyssal-upper-jaw" transform="rotate(-5 -2 -14)">
        <path d="M -2 -14 C 4 -26 13 -36 24 -36 L 32 -31 C 29 -21 19 -12 10 -8 Z" fill="#1d4ed8"/>
        <path d="M 19 -34 L 30 -31 L 23 -26 Z" fill="#62c8d0" stroke="none"/>
        <path d="M 5 -19 L 11 -23 L 10 -14 Z M 12 -23 L 18 -27 L 17 -18 Z M 20 -27 L 27 -29 L 24 -21 Z" fill="#cffff0" stroke-width="1.2"/>
      </g>
      <g data-part="abyssal-lower-jaw" transform="rotate(4 -5 4)">
        <path d="M -5 4 C 5 2 16 6 29 14 L 21 25 C 9 21 1 15 -5 10 Z" fill="#102d68"/>
        <path d="M 18 12 L 28 14 L 22 21 Z" fill="#3479bf" stroke-width="1.2"/>
        <path d="M 2 5 L 8 3 L 6 12 Z M 10 8 L 15 7 L 14 17 Z M 18 12 L 22 13 L 20 21 Z" fill="#cffff0" stroke-width="1.2"/>
      </g>
      <path d="M -19 -18 L -26 -30 L -20 -9 Z M -24 1 L -32 8 L -20 10 Z M -9 20 L -11 30 L -2 23 Z" fill="#0d293e" stroke="#379ba1" stroke-width="1.5"/>
      <path d="M -16 -18 L -11 -21 M -18 11 L -13 14" stroke="#5eead4" stroke-width="1.8" stroke-linecap="round"/>
    </g>
  </g>`;
  return hand==='claw'?claw:joanaAbyssHands({view,hand:'hook'})+claw;
}

function joanaAbyssFace(expression,monster=false) {
  const mouth=expression==='pain'?'M 51 38 Q 59 35 68 38 L 65 42 H 54 Z':expression==='game_over'?'M 51 40 Q 59 43 68 39 L 65 42 H 54 Z':`M 51 38 Q 59 ${expression==='angry'?35:37} 69 38 L 66 42 H 54 Z`;
  return `<g data-part="spectral-half-human-face" stroke="#050c19" stroke-linejoin="round">
    <path d="M 42 22 H 61 V 44 H 52 Q 43 40 42 33 Z" fill="#7acbb8" stroke-width="1.8"/>
    <path d="M 60 22 H 78 V 37 L 73 42 L 61 44 L 58 37 L 62 31 L 59 27 Z" fill="#1d4ed8" stroke-width="1.8"/>
    <path d="M 65 25 L 75 25 V 34 L 69 38 L 64 34 Z" fill="#3479bf" stroke-width=".8"/>
    <path d="M 65 26 L 73 26 L 66 30 Z" fill="#62c8d0" stroke="none"/>
    <path d="M 41 27 L 60 30" stroke-width="1.8"/>
    <rect x="46" y="27" width="10" height="8" rx="2" fill="#0b1725" stroke-width="1.4"/>
    <path d="M 48 29 L 52 29" stroke="#475569" stroke-width="1"/>
    <path d="M 57 31 L 55 36 H 59" fill="none" stroke="#378d83" stroke-width="1"/>
    ${monster?`<g data-part="abyssal-fanged-mouth"><path d="M 49 37 Q 60 ${expression==='angry'?33:35} 72 37 L 69 43 L 61 46 L 52 43 Z" fill="#07151f" stroke-width="1.2"/><path d="M 51 37 L 54 42 L 57 37 L 60 42 L 63 37 L 66 42 L 70 37 M 55 44 L 57 41 L 60 45 M 64 45 L 66 41 L 68 43" fill="#cffff0" stroke-width=".6"/></g>`:`<path d="${mouth}" fill="#10212d" stroke-width="1.1"/><path d="M 60 38 L 62 41 L 64 38 L 66 41 L 68 38" fill="#c6f6e8" stroke-width=".7"/>`}
  </g>`;
}

function renderJoanaAbyssSvg({view='front',expression='normal',armed=true,time=0,state='normal'}={}) {
  const back=view==='back',monster=state==='monster',ink='#050c19';
  const line=`stroke="${ink}" stroke-width="1.8" stroke-linejoin="round"`;
  const legs=`<g data-part="coral-peg-and-spectral-leg">
    <path d="M 43 89 H 51 V 108 L 48 112 H 44 L 41 108 Z" fill="#17323c" ${line}/>
    <path d="M 45 94 V 106" stroke="#46777a" stroke-width="1.3"/><path d="M 42 98 H 51 M 42 105 H 51" stroke="#2dd4bf" stroke-width="1.5"/>
    <path d="M 68 89 H 77 V 103 H 68 Z" fill="#63b5a6" ${line}/>
    <path d="M 67 100 H 78 V 106 L 82 108 V 112 H 67 Z" fill="#12233b" ${line}/><path d="M 69 103 H 76 M 69 110 H 80" stroke="#4f8b91" stroke-width="1.2"/>
  </g>`;
  const trousers=`<path data-part="officer-breeches-behind-coat" d="M 40 74 H 80 V 91 L 74 96 L 65 91 L 59 94 L 52 91 L 47 96 L 40 91 Z" fill="#475569" ${line}/><path d="M 51 80 V 91 M 69 80 V 91" stroke="#7a9aa3" stroke-width="1.2"/>`;
  const sleeves=`<g data-part="admiral-sleeves"><path d="M 39 46 Q 29 48 26 60 L 34 66 L 44 53 Z M 81 46 Q 90 48 94 60 L 86 66 L 76 53 Z" fill="#14263d" ${line}/><path d="M 27 58 L 35 62 M 86 62 L 93 58" stroke="#3aa99f" stroke-width="1.4"/></g>`;
  const coat=back?`<g data-part="admiral-coat-back">
    <path d="M 45 43 H 75 Q 84 44 86 54 L 82 72 L 90 97 L 82 96 L 79 103 L 73 99 L 69 102 L 60 85 L 51 102 L 47 99 L 41 103 L 38 96 L 30 97 L 38 72 L 34 54 Q 36 44 45 43 Z" fill="#101b32" ${line}/>
    <path d="M 43 47 Q 60 50 77 47 L 76 53 Q 60 56 44 53 Z" fill="#20384b" stroke="#315767" stroke-width="1"/>
    <path d="M 39 58 L 42 72 L 34 94 M 81 58 L 78 72 L 86 94 M 60 55 V 69" fill="none" stroke="#236a70" stroke-width="1.2"/>
    <path d="M 40 74 H 80 V 78 H 40 Z" fill="#091526" stroke="#315767" stroke-width="1"/>
    <g data-part="abyss-admiral-emblem" fill="none" stroke="#51bbad" stroke-width="1.2" stroke-linecap="round"><circle cx="60" cy="62" r="2"/><path d="M 60 64 V 72 M 56 66 H 64 M 52 68 Q 53 74 60 74 Q 67 74 68 68 M 52 68 L 51 71 M 68 68 L 69 71"/></g>
    <path d="M 42 82 L 38 95 M 78 82 L 82 95" stroke="#3da99f" stroke-width="1.1"/>
  </g>`:`<g data-part="admiral-coat-front" data-chest="covered">
    <path d="M 44 43 L 53 42 L 60 48 L 67 42 L 76 43 Q 85 45 87 55 L 82 72 L 90 97 L 82 96 L 79 103 L 73 99 L 69 102 L 60 83 L 51 102 L 47 99 L 41 103 L 38 96 L 30 97 L 38 72 L 33 55 Q 35 45 44 43 Z" fill="#14243c" ${line}/>
    <path d="M 61 49 L 77 47 L 80 57 L 76 73 L 85 95 L 77 98 L 70 96 L 61 81 Z" fill="#101b32"/>
    <path d="M 45 43 L 53 41 L 60 48 L 55 56 L 48 51 Z M 75 43 L 67 41 L 60 48 L 65 56 L 72 51 Z" fill="#33485c" stroke="#050c19" stroke-width="1.2"/>
    <path d="M 48 44 L 55 51 M 72 44 L 65 51 M 55 56 L 54 72 M 65 56 L 66 72" stroke="#38b4a5" stroke-width="1.2" fill="none"/>
    ${[59,66].map(y=>`<circle cx="56" cy="${y}" r="1.25" fill="#67d6bf"/><circle cx="64" cy="${y}" r="1.25" fill="#67d6bf"/>`).join('')}
    <path d="M 40 73 H 80 V 78 H 40 Z" fill="#091526" stroke="#315767" stroke-width="1"/><rect x="56" y="72" width="8" height="7" rx="1" fill="#475569" stroke="#050c19" stroke-width="1.2"/><path d="M 59 74 H 62 V 77 H 59 Z" fill="#0d9488"/>
    <path d="M 43 80 L 38 94 M 77 80 L 82 94 M 36 94 L 40 96 M 80 96 L 84 94" stroke="#3da99f" stroke-width="1.2"/>
  </g>`;
  const epaulettes=`<g data-part="ghostly-admiral-epaulettes" ${line}><path d="M 34 46 L 44 43 L 48 48 L 44 52 L 33 51 Z M 86 46 L 76 43 L 72 48 L 76 52 L 87 51 Z" fill="#33485c"/><path d="M 35 48 L 44 46 M 76 46 L 85 48" stroke="#67e8ce" stroke-width="1.4"/><path d="M 35 52 V 56 M 39 53 V 57 M 43 52 V 55 M 77 52 V 55 M 81 53 V 57 M 85 52 V 56" stroke="#2b8d88" stroke-width="1.3"/></g>`;
  const neck=`<path d="M 54 39 H 66 V 47 L 60 50 L 54 47 Z" fill="#63b5a6" ${line}/><path d="M 60 40 H 66 V 46 L 60 49 Z" fill="#17398c"/>`;
  const head=back?`<g data-part="back-head"><path d="M 42 20 H 78 V 37 Q 76 45 67 46 H 52 Q 44 44 42 37 Z" fill="#0b333c" ${line}/><path d="M 60 21 H 78 V 37 L 72 43 L 61 45 Z" fill="#17398c" stroke="#050c19" stroke-width="1.2"/></g>`:joanaAbyssFace(expression,monster);
  const hat=`<g data-part="tattered-admiral-bicorne">
    <path d="M 41 20 Q 43 9 53 11 L 60 14 L 67 11 Q 77 9 80 20 Z" fill="#132941" ${line}/>
    <path d="M 34 19 L 43 17 L 52 21 L 60 18 L 69 21 L 78 17 L 86 20 L 82 27 L 75 25 L 68 28 L 60 25 L 52 28 L 45 25 L 37 27 Z" fill="#0b1529" ${line}/>
    <path d="M 38 21 L 44 20 L 52 24 L 60 21 L 68 24 L 77 20 L 82 22" fill="none" stroke="#42baa9" stroke-width="1.2"/>
    ${back?'':`<path d="M 57 17 L 60 15 L 63 17 L 60 20 Z" fill="#a8eee0" stroke="#050c19" stroke-width=".8"/>`}
  </g>`;
  const lantern=`<g data-part="anglerfish-eye-lantern">
    <path d="M 76 22 L 78 14 L 81 10" fill="none" stroke="#050c19" stroke-width="4.5"/><path d="M 76 22 L 78 14 L 81 10" fill="none" stroke="#2563b5" stroke-width="2.2"/>
    <circle data-lantern-glow="true" cx="81" cy="7" r="5.7" fill="#2dd4bf" opacity=".14"/>
    <circle cx="81" cy="7" r="3.8" fill="#184458" stroke="#050c19" stroke-width="1.5"/><circle cx="81" cy="7" r="2.1" fill="${back?'#5ccfb6':'#c9ffeb'}"/><circle cx="82" cy="6" r=".8" fill="#f2fff8"/>
  </g>`;
  const corruption=monster?`<g data-part="abyssal-coral-corruption" stroke="#050c19" stroke-linejoin="round"><path d="M 81 54 L 86 44 L 87 50 L 93 47 L 87 58 Z M 81 73 L 88 77 L 86 84 L 82 78 Z" fill="#122c49" stroke-width="1.3"/><path d="M 84 52 L 86 49 M 84 77 L 86 79 M 73 84 L 77 88 L 74 92 L 79 96" fill="none" stroke="#57cfc2" stroke-width=".9"/></g>`:'';
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 120 120" width="360" height="360" role="img" aria-label="Joana Almirante do Abismo${monster?' monstruosa':''} — ${back?'costas':'frente'}" data-skin="joana-abyss" data-state="${monster?'monster':'normal'}">
    <ellipse cx="60" cy="113" rx="31" ry="3" fill="#061b28" opacity=".5"/><ellipse cx="60" cy="114" rx="23" ry="1.2" fill="none" stroke="#247972" stroke-width=".6"/>
    ${legs}${trousers}${sleeves}${coat}${epaulettes}${corruption}${neck}${back?head+joanaAbyssDreads({view,time}):joanaAbyssDreads({view,time})+head}${back?lantern+hat:hat+lantern}${armed?(monster?joanaAbyssMonsterHands({view,time}):joanaAbyssHands({view})):''}
  </svg>`;
}

// Standalone native SVG animation, independent of the lab's JavaScript clock.
function renderJoanaAbyssAnimatedSvg(options={}) {
  const view=options.view||'front';
  const keyframes=[0,1.5,3,4.5,6].map(time=>joanaAbyssDreadFrame(view,time));
  let svg=renderJoanaAbyssSvg({...options,time:0}).replace(/<path data-dread="(\d+)" data-layer="([^"]+)"([^>]+)\/>/g,(tag,index,layer,attributes)=>{
    const values=keyframes.map(frame=>frame[Number(index)][layer==='branch'?'branch':'d']).join(';');
    return `<path data-dread="${index}" data-layer="${layer}"${attributes}><animate attributeName="d" values="${values}" dur="6s" repeatCount="indefinite" calcMode="spline" keyTimes="0;.25;.5;.75;1" keySplines=".45 0 .55 1;.45 0 .55 1;.45 0 .55 1;.45 0 .55 1"/></path>`;
  });
  const tentacles=[0,1.5,3,4.5,6].map(time=>joanaAbyssTentacleFrame(view,time));
  const animation=(attribute,values)=>`<animate attributeName="${attribute}" values="${values.join(';')}" dur="6s" repeatCount="indefinite" calcMode="spline" keyTimes="0;.25;.5;.75;1" keySplines=".45 0 .55 1;.45 0 .55 1;.45 0 .55 1;.45 0 .55 1"/>`;
  svg=svg.replace(/<path data-tentacle="(\d+)" data-layer="([^"]+)"([^>]+)\/>/g,(tag,index,layer,attributes)=>`<path data-tentacle="${index}" data-layer="${layer}"${attributes}>${animation('d',tentacles.map(frame=>frame[Number(index)].d))}</path>`);
  return svg.replace(/<circle data-sucker="(\d+)" data-cup="(\d+)"([^>]+)\/>/g,(tag,index,cup,attributes)=>`<circle data-sucker="${index}" data-cup="${cup}"${attributes}>${animation('cx',tentacles.map(frame=>frame[Number(index)].suckers[Number(cup)].x))}${animation('cy',tentacles.map(frame=>frame[Number(index)].suckers[Number(cup)].y))}</circle>`);
}

return o=>{if(o.phase)o.phase=Number(o.phase);return o.animate?renderJoanaAbyssAnimatedSvg(o):renderJoanaAbyssSvg(o);};};const rawDrawJoana=makeArt();const skinId=2;const drawJoana=o=>refineJoanaSkin(rawDrawJoana(o),o,skinId);

// Expressões da Joana: as bocas antigas mudavam 1-2 px e os olhos ficam sob tapa-olho, chapéu ou óculos.
// Cada expressão troca a boca por uma grande (mesmo vocabulário da dor do jogo, reactions.js),
// com sobrancelha, suor e o olho do pedúnculo de siri reagindo na original.
const JOANA_MOUTH_D=new Set(['M 52 37 Q 60 34 68 37 L 65 40 H 55 Z','M 55 37 H 65','M 52 38 Q 60 41 68 37','M 51 38 Q 60 35 69 38','M 51 38 Q 60 36 69 38','M 55 40 H 65',
 'M 51 38 Q 59 35 68 38 L 65 42 H 54 Z','M 51 40 Q 59 43 68 39 L 65 42 H 54 Z','M 51 38 Q 59 35 69 38 L 66 42 H 54 Z','M 51 38 Q 59 37 69 38 L 66 42 H 54 Z',
 'M 51 37 Q 59 34 68 37 L 65 42 H 54 Z','M 51 40 L 58 41 L 67 38 L 65 42 H 54 Z','M 51 38 Q 59 35 68 38 L 65 41 H 54 Z','M 51 38 Q 59 37 68 38 L 65 41 H 54 Z','M 60 38 L 63 40 L 65 38 L 67 40',
 'M 63 39 L 67 39 L 65 42 Z']);
function joanaExpression(expression,skin){
 const ink='#151c23',brows=skin!==1;
 if(expression==='pain'){
  let drops='';for(let i=0;i<4;i++){const s=4.2,dx=s*(1.2+i*.9),dy=-s*(.6+i*.25)+i*i*s*.12;drops+=`<circle cx="${(72+dx).toFixed(1)}" cy="${(34+dy).toFixed(1)}" r="${(s*(.32-i*.04)).toFixed(2)}" fill="${i%2?'#ffffff':'#9be7ff'}" stroke="${ink}" stroke-width=".35"/>`;}
  return `<g data-joana-expression="pain">${brows?`<path d="M 44 25.5 Q 49 22.6 56 25" fill="none" stroke="${ink}" stroke-width="1.7" stroke-linecap="round"/><path d="M 51 23 L 53 24.6" stroke="${ink}" stroke-width=".9" stroke-linecap="round"/>`:''}
   <path d="M 50 41.8 Q 59 36.4 69 42.2 L 68 45 Q 59 40.4 51 44.4 Z" fill="#f8f4e6" stroke="${ink}" stroke-width="1.2" stroke-linejoin="round"/>
   <path d="M 50.5 43.1 Q 59 38.4 68.5 43.6" fill="none" stroke="${ink}" stroke-width=".8"/><path d="M 54 40.3 V 43.4 M 58 39.2 V 42.3 M 62 39.3 V 42.3 M 65.6 40.6 V 43.5" stroke="${ink}" stroke-width=".55"/>${skin===0?'<path d="M 54.4 40 Q 56.2 39.2 57.6 39.3 L 57.6 41.3 Q 56 41.5 54.6 42.1 Z" fill="#e6b83f" stroke="#151c23" stroke-width=".45"/>':''}
   <g data-part="pain-sweat-drops">${drops}</g></g>`;
 }
 if(expression==='angry'){
  return `<g data-joana-expression="angry">${brows?`<path d="M 43.5 24.6 L 56.5 28.4" stroke="${ink}" stroke-width="2.4" stroke-linecap="round"/><path d="M 57.5 26 L 59.5 24.5 M 58.2 28 L 61 27.4" stroke="${ink}" stroke-width=".9" stroke-linecap="round"/>`:''}
   <path d="M 50 37.6 L 68.6 36.6 L 67.6 44 Q 59 46.4 51 44.2 Z" fill="#2a0a12" stroke="${ink}" stroke-width="1.3" stroke-linejoin="round"/>
   <path d="M 51.2 38.4 L 67.6 37.5 L 67.4 40.2 L 51.4 40.8 Z" fill="#f8f4e6"/><path d="M 51.8 42.4 L 67 41.8 L 66.8 43.5 Q 59 45.3 52 43.7 Z" fill="#f8f4e6"/>
   <path d="M 55 38.2 V 40.6 M 59 38 V 40.5 M 63 37.8 V 40.3 M 55.6 42.3 V 44.2 M 59.4 42.1 V 44.6 M 63.2 42 V 44.2" stroke="${ink}" stroke-width=".6"/>${skin===0?'<path d="M 55.2 38.3 H 58.8 V 40.5 H 55.2 Z" fill="#e6b83f" stroke="#151c23" stroke-width=".5"/>':''}</g>`;
 }
 if(expression==='game_over'){
  return `<g data-joana-expression="game_over">${brows?`<path d="M 44 26.5 Q 50 24.5 56 26.8" fill="none" stroke="${ink}" stroke-width="1.4" stroke-linecap="round"/>`:''}
   <path d="M 51 39.4 Q 55 37.6 59 39.2 Q 64 37.4 68 39.8 L 66.4 44.2 Q 59 46.8 52.4 44 Z" fill="#2a0a12" stroke="${ink}" stroke-width="1.3" stroke-linejoin="round"/>
   <path d="M 59 42.6 Q 60 50.4 65 49.4 Q 67.8 48 65.6 43 Z" fill="#ff5c7a" stroke="${ink}" stroke-width=".9" stroke-linejoin="round"/><path d="M 62.6 44 L 63.4 47.6" stroke="#c23b57" stroke-width=".7"/></g>`;
 }
 return '';
}
function joanaStalkEye(svg,expression){
 const pupil=/<ellipse cx="80.5" cy="10" rx="1.3" ry="2.1" fill="[^"]+"\/>/;
 if(expression==='pain')return svg.replace(pupil,'<path d="M 78.2 8.2 L 81.6 10 L 78.2 11.8" fill="none" stroke="#151c23" stroke-width="1.1" stroke-linecap="round" stroke-linejoin="round"/>');
 if(expression==='angry')return svg.replace(pupil,m=>m+'<path d="M 76.6 7.6 L 83.4 10.2" stroke="#151c23" stroke-width="1.6" stroke-linecap="round"/>');
 return svg;
}
// Gallery-only silhouette cleanup. Preserve the approved game files and equipment art.
function refineJoanaSkin(svg,o,skin){
 const expr=!o.drinking&&!o.mouthOpen&&o.state!=='monster'&&o.view!=='back'&&['pain','angry','game_over'].includes(o.expression)?o.expression:'';
 if(expr&&skin===0)svg=joanaStalkEye(svg,expr);
 if(![1,3].includes(skin)&&!expr)return svg;
 const removed=new Set(['human-arms','torn-sleeves','industrial-arm-sockets','copper-diver-shoulders']);
 function clean(s){
  const tag=s.match(/^<[^>]+>/)?.[0]||'',name=tag.match(/data-part="([^"]+)"/)?.[1]||'';
  if([1,3].includes(skin)&&removed.has(name))return '';
  if(!/^<(?:g|svg)\b/.test(tag))return s;
  let children=nodes(inside(s));
  if([1,3].includes(skin)&&['ruby-king-crab-claw','hydraulic-dredger-claw'].includes(name)){
   // These leading polygons were forearm connectors, not part of the actual claw.
   const start=children.findIndex(c=>/^<g\b/.test(c));if(start>=0)children=children.slice(start);
  }
  if([1,3].includes(skin)&&name==='human-hook-hand')children=children.filter(c=>!c.includes('d="M 86 57 H 94 V 63 H 86 Z"'));
  const faceGroup=expr&&!/data-monster-face/.test(tag)&&(name==='human-crab-face'||name==='spectral-half-human-face'||name==='half-human-half-copper-face'||(skin===1&&name==='human-head'));
  if(faceGroup)children=children.filter(c=>!(/^<path\b/.test(c)&&JOANA_MOUTH_D.has(c.match(/\sd="([^"]+)"/)?.[1]))&&!/data-part="open-mouth"/.test(c.match(/^<[^>]+>/)?.[0]||''));
  let body=children.map(clean).join('')+(faceGroup?joanaExpression(expr,skin):'');
  if(skin===3&&name==='steam-transmission-pipes')body=body.replaceAll('M 72 46 Q 78 44 80 49 L 82 57 M 43 46 Q 34 48 35 60','M 72 46 Q 77 45 77 50 V 57 M 46 46 Q 43 48 43 60');
  if(skin===1&&name==='human-head'&&(o.expression==='normal'||!o.expression||o.mouthOpen)){
   const smileShape='M 52 36 Q 58 38 66 37 Q 64 41 55 41 L 52 40 Z';
   const smile=`<g data-mouth="smile"><defs><clipPath id="joana-fisher-smile-clip"><path d="${smileShape}"/></clipPath></defs><path d="${smileShape}" fill="#f1e6cc"/><path d="M 53 39 Q 58 40 64 38 L 62 40 Q 58 42 54 41 Z" fill="#563129" clip-path="url(#joana-fisher-smile-clip)"/><path data-mouth-outline="true" d="${smileShape}" fill="none" stroke="#111c24" stroke-width="1.2" stroke-linejoin="round"/><path d="M 51 37 L 52 36 M 65 37 L 67 36" fill="none" stroke="#99553b" stroke-width=".8" stroke-linecap="round"/></g>`;
   const mouth=o.mouthOpen?'<g data-mouth="drinking"><path d="M 52 37 Q 60 35 68 37 L 66 43 Q 60 46 54 43 Z" fill="#563129" stroke="#111c24" stroke-width="1.5" stroke-linejoin="round"/></g>':smile;
   body=body.replace(/<path d="M 51 38 Q 60 (?:35|36) 69 38"[^>]*\/>/,mouth).replace(/<path d="M 55 40 H 65"[^>]*\/>/,'');
  }
  return tag+body+(s.match(/<\/[^>]+>\s*$/)?.[0]||'');
 }
 return clean(svg);
}

const I=[1,0,0,1,0,0],n=x=>Number(x.toFixed(5)),matrix=m=>'matrix('+m.map(n).join(' ')+')';
function mul(a,b){return[a[0]*b[0]+a[2]*b[1],a[1]*b[0]+a[3]*b[1],a[0]*b[2]+a[2]*b[3],a[1]*b[2]+a[3]*b[3],a[0]*b[4]+a[2]*b[5]+a[4],a[1]*b[4]+a[3]*b[5]+a[5]];}
function inverse(m){const d=m[0]*m[3]-m[1]*m[2];return Math.abs(d)<1e-8?I:[m[3]/d,-m[1]/d,-m[2]/d,m[0]/d,(m[2]*m[5]-m[3]*m[4])/d,(m[1]*m[4]-m[0]*m[5])/d];}
const wrap=(s,m=I,a='')=>`<g transform="${matrix(m)}" ${a}>${s}</g>`;
const lerp=(a,b,p)=>a+(b-a)*p,smooth=p=>{p=Math.max(0,Math.min(1,p));return p*p*(3-2*p);};
// Sample both exports at the same continuous time, instead of floor/round on separate clocks.
function sample(T,p){const k=Math.max(0,Math.min(T.length-1,p*(T.length-1))),i=Math.floor(k),q=k-i;return blend(T[i],T[Math.min(i+1,T.length-1)],q);}
function blend(a,b,q){return Array.isArray(a)?a.map((v,j)=>blend(v,b[j],q)):typeof a==='number'?lerp(a,b,q):q<.5?a:b;}
function punchTrack(T,p,rest){
 const samples=[-2,-1,0,1,2].map(j=>sample(T,Math.max(0,Math.min(1,p+j/(T.length-1))))),out=sample(T,p),envelope=smooth(p/.12)*(1-smooth((p-.76)/.24));
 // Captured screen-space hand tracks include camera shake and a one-frame guard reset.
 // Filter that capture noise, then recover continuously to the redesigned guard.
 for(let j=0;j<5;j++)out[j]=lerp(rest[j],samples.reduce((a,s,i)=>a+s[j]*(3-Math.abs(i-2))/9,0),envelope);
 for(let i=5;i<7;i++)for(let j=0;j<5;j++)out[i][j]=lerp(rest[i][j],samples.reduce((a,s,k)=>a+s[i][j]*(3-Math.abs(k-2))/9,0),envelope);
 return out;
}
function bottle(x,y,r,level=1,opacity=1){return `<g data-part="signature-rum" data-liquid="${n(level)}" transform="translate(${n(x)} ${n(y)}) rotate(${n(r)})" opacity="${n(opacity)}" stroke="#151c23" stroke-linejoin="round"><path d="M -1.8 -15 H 1.8 V -9 L 4.8 -6 V 8 Q 0 10 -4.8 8 V -6 L -1.8 -9 Z" fill="#336b4c" stroke-width="1.5"/>${level>.01?`<path d="M -3.7 ${n(7-12*level)} H 3.7 V 7 H -3.7 Z" fill="#b7762b"/>`:''}<path d="M -3.8 -2 H 3.8 V 4 H -3.8 Z" fill="#e8d7a5" stroke-width=".8"/><text x="0" y="2.1" text-anchor="middle" font-family="Arial" font-weight="bold" font-size="3.3" fill="#5a3128" stroke="none">RUM</text><path d="M -1 -12 V -9 M -3 -5 V -3" stroke="#a0bea0" stroke-width=".8"/></g>`;}
function largeBottle(x,y,r,level,opacity=1){const a=r*Math.PI/180;return `<g data-bottle-tip="${n(x+24*Math.sin(a))} ${n(y-24*Math.cos(a))}" transform="translate(${n(x)} ${n(y)}) rotate(${n(r)}) scale(1.6)">${bottle(0,0,0,level,opacity)}</g>`;}
function beerMug(x,y,r,level=1,opacity=1){const a=r*Math.PI/180,foamY=12-20*level;return `<g data-part="signature-beer-mug" data-liquid="${n(level)}" data-mug-lip="${n(x+12*Math.sin(a))} ${n(y-12*Math.cos(a))}" transform="translate(${n(x)} ${n(y)}) rotate(${n(r)})" opacity="${n(opacity)}" stroke="#111c24" stroke-linejoin="round"><path d="M 9 -7 H 14 Q 20 -7 20 -1 V 5 Q 20 10 14 10 H 9 V 6 H 14 Q 16 6 16 3 V 0 Q 16 -3 13 -3 H 9 Z" fill="#b7c9bd" stroke-width="1.5"/><path d="M -9 -12 H 9 V 12 Q 0 16 -9 12 Z" fill="#718e83" stroke-width="1.6"/><defs><clipPath id="joana-beer-glass"><path d="M -7.5 -10.5 H 7.5 V 11.5 Q 0 14 -7.5 11.5 Z"/></clipPath></defs><g clip-path="url(#joana-beer-glass)" stroke="none"><path d="M -8 ${n(foamY)} H 8 V 14 H -8 Z" fill="#d8a035"/>${level>.03?`<path d="M -9 ${n(foamY)} Q -6 ${n(foamY-5)} -2 ${n(foamY-2)} Q 2 ${n(foamY-6)} 5 ${n(foamY-2)} Q 9 ${n(foamY-4)} 10 ${n(foamY)} V ${n(foamY+2)} H -9 Z" fill="#f7efd3"/>`:''}<path d="M -6 -8 V 9 H -4 V -8 Z M 5 -7 V 9 H 6 V -7 Z" fill="#e5efe0" opacity=".5"/></g><path d="M -9 -12 Q 0 -14 9 -12 M -8 12 Q 0 15 8 12" fill="none" stroke="#d1dfce" stroke-width="1.3"/></g>`;}
function signature(p,anchor,back,mouth,{beer=false}={}){
 const pinch=[13*(back?-1:1),-5],restGrip=[anchor[0]+pinch[0],anchor[1]+pinch[1]],restFacing=1;
 // Grip is the contact between the pincers, not the wrist or the claw's center.
 const drinkAngle=beer?-35:-72,tipLength=beer?12:24,tilt=drinkAngle*Math.PI/180,drink=[mouth[0]-tipLength*Math.sin(tilt),mouth[1]+tipLength*Math.cos(tilt)],outside=[restGrip[0]+24,restGrip[1]-2];
 const keys=[[0,...restGrip,0,restFacing],[.045,...outside,0,restFacing],[.065,...outside,0,restFacing],[.19,55,58,0,back?1:-1],[.22,55,58,0,back?1:-1],[.30,108,62,-8,back?1:-1],[.34,108,62,-8,back?1:-1],[.44,...drink,drinkAngle,back?1:-1],[.68,...drink,drinkAngle,back?1:-1],[.76,92,61,0,back?1:-1],[.81,107,35,55,1],[.84,117,31,85,1],[.97,...restGrip,0,restFacing],[1,...restGrip,0,restFacing]];
 let j=0;while(j<keys.length-2&&p>keys[j+1][0])j++;
 const a=keys[j],b=keys[j+1],u=Math.max(0,Math.min(1,(p-a[0])/(b[0]-a[0]))),q=j===0?u*u*u*(u*(u*6-15)+10):smooth(u),v=a.slice(1).map((x,i)=>lerp(x,b[i+1],q));
 const [gx,gy,r,flip]=v,ang=r*Math.PI/180,c=Math.cos(ang),s=Math.sin(ang),sx=flip;
 const m=[c*sx,s*sx,-s,c,gx-c*sx*(anchor[0]+pinch[0])+s*(anchor[1]+pinch[1]),gy-s*sx*(anchor[0]+pinch[0])-c*(anchor[1]+pinch[1])];
 let rum='',stage=p<.065?'clear-side':p<.19?'reach':p<.22?'grasp':p<.34?'pull':p<.44?'raise':p<.68?'drink':p<.76?'lower':p<.84?'wind':p<.97?'throw':'return',prop=beer?beerMug:largeBottle;
 if(p>=.19&&p<.84)rum=prop(gx,gy,r,1-smooth((p-.44)/.24));
 if(p>=.84&&p<.97){const t=(p-.84)/.13;rum=prop(117+t*40,31-t*45+30*t*t,85+t*300,0,1-smooth((t-.55)/.45));}
 // Keep the actual hand and held bottle behind the torso until fully outside its silhouette.
 // The depth switch happens during the lateral hold, not while crossing the clothing.
 return {m,rum,stage,behind:p>=.055&&p<.32,held:p>=.19&&p<.84,close:smooth((p-.16)/.045)*(1-smooth((p-.825)/.025))};
}
function atPoint(m,x,y){return[m[0]*x+m[2]*y+m[4],m[1]*x+m[3]*y+m[5]];}
function handPose(anchor,x,y,degrees){const a=degrees*Math.PI/180,c=Math.cos(a),s=Math.sin(a);return[c,s,-s,c,x-c*anchor[0]+s*anchor[1],y-s*anchor[0]-c*anchor[1]];}
function mechSignature(p,back){
 const lift=smooth(p/.16),approach=smooth((p-.16)/.12),shock=smooth((p-.515)/.055)*(1-smooth((p-.70)/.23)),recover=1-smooth((p-.70)/.23);
 const claw=handPose([94,66],lerp(94,82,lift)+29*shock,lerp(66,63,lift)-18*shock,lerp(0,back?20:-20,lift)+65*shock),contact=atPoint(claw,back?102:86,62);
 const angle=-25*approach,rad=angle*Math.PI/180,dx=12*Math.cos(rad)+Math.sin(rad),dy=12*Math.sin(rad)-Math.cos(rad),tap=p>=.28&&p<.515?4*(1-Math.cos((p-.28)/.235*Math.PI*4)):0;
 let hx=lerp(25,contact[0]-dx,approach)-tap,hy=lerp(64,contact[1]-dy,approach);
 hx=lerp(hx,27,shock);hy=lerp(hy,83,shock);
 const hook=handPose([25,64],lerp(25,hx,recover),lerp(64,hy,recover),angle*recover-25*shock);
 // Both hands return to their actual approved rest anchors, not the repair pose.
 const finalClaw=claw.map((v,i)=>lerp(I[i],v,recover));
 let fx='',sparks='';const burst=p>=.515&&p<.72,smallTap=p>=.28&&p<.515&&tap<1.2;
 if(burst||smallTap){const u=burst?(p-.515)/.205:tap,origin=atPoint(finalClaw,back?102:86,62);for(let j=0;j<7;j++){const a=j*Math.PI*2/7+.3,dist=burst?4+u*24:4+j%3*2,len=burst?5*(1-u)+2:3,x=origin[0]+Math.cos(a)*dist,y=origin[1]+Math.sin(a)*dist;sparks+=`<path d="M ${n(x)} ${n(y)} l ${n(Math.cos(a)*len)} ${n(Math.sin(a)*len)}" stroke="${j%2?'#ffad43':'#ffe9a2'}" stroke-width="${burst?1.8:1.2}" stroke-linecap="round"/>`;}
  fx+=`<g data-part="repair-sparks" opacity="${burst?n(1-(p-.515)/.205):1}">${sparks}</g>`;
 }
 if(p>=.515&&p<.88){const origin=atPoint(finalClaw,back?102:86,62);let steam='';for(let j=0;j<5;j++){const u=(p-.515-j*.025)/.24;if(u<=0||u>=1)continue;const x=origin[0]+Math.sin(j*2.3+u*2)*(3+u*8),y=origin[1]-u*(25+j*2),r=2+u*5;steam+=`<g transform="translate(${n(x)} ${n(y)})" opacity="${n(Math.sin(u*Math.PI)*.65)}"><circle cx="0" cy="0" r="${n(r)}" fill="#dce5dc"/><circle cx="${n(-r*.7)}" cy="${n(r*.2)}" r="${n(r*.65)}" fill="#abbdb7"/><circle cx="${n(r*.6)}" cy="${n(-r*.4)}" r="${n(r*.6)}" fill="#edf2e7"/></g>`;}fx+=`<g data-part="repair-steam">${steam}</g>`;}
 if(shock>.1)fx+=`<g data-part="repair-startle" opacity="${n(shock)}" stroke="#f1ce79" stroke-width="1.7" stroke-linecap="round"><path d="M 34 21 L 29 17 M 37 16 L 35 10 M 43 15 L 44 9"/></g>`;
 const recoil=handPose([60,83],60-4*shock,83-4*shock,-5*shock);
 return{hands:[hook,finalClaw],recoil,fx,stage:p<.16?'inspect':p<.28?'approach':p<.515?'poke':p<.70?'startle':p<.93?'recover':'return',shock};
}
function closePincers(hand,amount){
 const tag=hand.match(/^<g\b[^>]*>/)?.[0]||'';if(!tag||amount===0)return hand;
 if(skinId===1&&tag.includes('data-part="fishing-hook"')){
  // An ordinary metal fishing hook has no articulated jaw. Its entire approved
  // silhouette travels with the brown hand; never invent a second pincer.
  return hand;
 }
 const local=/translate\(94 66\) scale\(/.test(tag),pieces=nodes(inside(hand));
 return tag+pieces.map((piece,i)=>{
  if(local&&skinId===0&&[2,3].includes(i))return `<g data-jaw-close="${amount}" data-moving-jaw="${i===2?'upper':'lower'}" transform="rotate(${i===2?17*amount:-13*amount} ${i===2?'-6 -13':'-4 5'})">${piece}</g>`;
  if(local&&skinId===0&&i===4){const d=piece.match(/\bd="([^"]+)"/);if(d){const paths=d[1].split(/(?=M )/).filter(Boolean);return paths.map((s,j)=>`<g data-jaw-teeth="${j<2?'upper':'lower'}" transform="rotate(${j<2?17*amount:-13*amount} ${j<2?'-6 -13':'-4 5'})">${piece.replace(d[1],s)}</g>`).join('');}}
  const pieceTag=piece.match(/^<[^>]+>/)?.[0]||'',jawTurn=pieceTag.match(/transform="rotate\((-?[\d.]+) (-?[\d.]+) (-?[\d.]+)\)"/);
  if(local&&/^<g\b/.test(pieceTag)&&jawTurn&&(+jawTurn[3]===-14||+jawTurn[3]===4)){
   const lower=+jawTurn[3]===4,abyss=/data-part="abyssal-(?:upper|lower)-jaw"/.test(pieceTag),angle=abyss?(lower?-18:22.5):(lower?-8.5:14);
   return `<g data-jaw-close="${amount}" data-moving-jaw="${lower?'lower':'upper'}" transform="rotate(${angle*amount} ${lower?'-5 4':'-2 -14'})">${piece}</g>`;
  }
  if(local&&skinId===3&&/^<path/.test(piece)&&/d="M -[45] -[19]0? L/.test(piece)){const d=piece.match(/\bd="([^"]+)"/);if(d)return d[1].split(/(?=M )/).filter(Boolean).map((s,j)=>`<g data-moving-piston="${j}" transform="rotate(${(j?-8.5:14)*amount} ${j?'-5 4':'-2 -14'})">${piece.replace(d[1],s)}</g>`).join('');}
  return /^<g\b/.test(piece)?closePincers(piece,amount):piece;
 }).join('')+'</g>';
}
function gripBottle(hand,rum,m){
 const tag=hand.match(/^<g\b[^>]*>/)?.[0]||'',local=tag.match(/translate\(94 66\) scale\(([-.\d]+) ([-.\d]+)\)/);
 if(local){const pieces=nodes(inside(hand)),index=pieces.findIndex((s,i)=>i>=2&&(/^<g\b/.test(s)||i===2&&pieces.length<12));
  const insert=index<0?2:index,b=wrap(wrap(rum,inverse(m)),inverse([+local[1],0,0,+local[2],94,66]),'data-grip="between-pincers"');
  return {svg:tag+pieces.slice(0,insert).join('')+b+pieces.slice(insert).join('')+'</g>',found:true};
 }
 if(tag){let found=false;const children=nodes(inside(hand)).map(s=>{if(found||!/^<g\b/.test(s))return s;const r=gripBottle(s,rum,m);found=r.found;return r.svg;});return {svg:tag+children.join('')+'</g>',found};}
 return {svg:hand,found:false};
}
// Work on complete authored elements: nested hat, hair and clothing order stays intact.
function nodes(svg){
 const result=[],re=/<\/?([\w:-]+)\b[^>]*>/g;let start=-1,depth=0,m;
 while((m=re.exec(svg))){const closing=m[0].startsWith('</'),self=m[0].endsWith('/>');
  if(!closing&&depth===0)start=m.index;
  if(!closing&&!self)depth++;
  if(closing)depth--;
  if(depth===0&&start>=0){result.push(svg.slice(start,re.lastIndex));start=-1;}
 }return result;
}
const inside=s=>s.replace(/^\s*<[^>]+>/,'').replace(/<\/[^>]+>\s*$/,'');
function firstPoint(s){const tag=s.match(/^<[^>]+>/)?.[0]||'',d=tag.match(/(?:d|points)="[Mm\s]*([-\d.]+)[,\s]+([-\d.]+)/);return d?[+d[1],+d[2]]:[Number(tag.match(/\b(?:x|x1|cx)="([-\d.]+)"/)?.[1]??60),Number(tag.match(/\b(?:y|y1|cy)="([-\d.]+)"/)?.[1]??0)];}
function posedLegs(s,pose){
 const open=s.match(/^<g\b[^>]*>/)?.[0]||'<g>',children=nodes(inside(s));
 return open+children.map(child=>{
  const d=child.match(/^<path\b[^>]*d="([^"]+)"/),pieces=d?d[1].trim().split(/\s*(?=M\s*[-\d])/).filter(Boolean):[];
  if(pieces.length>1)return pieces.map(p=>posedLegs('<g>'+child.replace(d[1],p)+'</g>',pose)).join('');
  if(/^<g\b/.test(child)&&!child.includes('clip-path='))return posedLegs(child,pose);
  const [x,y]=firstPoint(child),slot=y>=87?(x<60?'legL':'legR'):'torso';
  return wrap(child,pose[slot]||I,`data-joana-limb="${slot}"`);
 }).join('')+'</g>';
}
const handNames=new Map([['pirate-hook',0],['pirate-crab-claw',1],['harpoon',0],['fishing-hook',1],['ruby-king-crab-claw',1],['black-coral-hook',0],['glass-blue-lobster-claw',1],['abyssal-maw-claw',1],['tempered-steel-dredging-hook',0],['hydraulic-dredger-claw',1]]);
function parts(o,pose={},turn=0){
 const svg=drawJoana({...o,armed:true,animate:skinId===0,partsOnly:false}),children=nodes(inside(svg)),hands=['',''];let body='',defs='';
 for(let s of children){
  const tag=s.match(/^<[^>]+>/)?.[0]||'',name=tag.match(/data-part="([^"]+)"/)?.[1]||'';
  if(/^<(defs|style)\b/.test(tag)){defs+=s;continue;}
  if(/^<ellipse\b/.test(tag)&&/cy="11[34]"/.test(tag))continue;
  if(handNames.has(name)){hands[handNames.get(name)]+=s;continue;}
  let slot='torso';
  if(/tricorn|souwester|bicorne|headwear/.test(name))slot='bandana';
  else if(/eyestalk|eye-lantern|periscope-eye/.test(name))slot='stalk';
  else if(/dread|human-hair/.test(name))slot='hair';
  else if(/head|face/.test(name))slot='head';
  else if(/pirate-legs|human-legs|monster-legs|coral-peg-and-spectral-leg/.test(name)){body+=posedLegs(s,pose);continue;}
  else if(name==='pneumatic-spring-leg')slot='legL';
  else if(!name&&firstPoint(s)[1]>=87)slot=firstPoint(s)[0]<60?'legL':'legR';
  if(turn&&slot==='head')s=`<g data-hit-profile="${n(turn)}" transform="translate(${60+turn*4} 0) scale(${1-Math.abs(turn)*.26} 1) translate(-60 0)">${s}</g>`;
  body+=wrap(s,pose[slot]||(['head','hair','bandana','stalk'].includes(slot)?pose.head:pose.torso)||I,`data-joana-part="${slot}"`);
 }
 return {body,defs,hands};
}
function spin(o){
 const p=((o.time||0)%2.4)/2.4,a=p*Math.PI*4+(o.view==='back'?Math.PI:0),sector=Math.floor((a%(Math.PI*2))/(Math.PI/4)+.5)%8,frame=[0,1,2,3,4,3,2,1][sector],view=frame>=3?'back':'front',art=parts({...o,view,expression:'game_over'}, {},[0,.6,1,.6,0][frame]),sx=[1,.82,.5,.82,1][frame]*(sector>4?-1:1),flight=o.action==='eject';
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-50 -65 220 210" data-combat="${o.action}" data-sector="${sector}" data-angle-frame="${frame}">${art.defs}<g transform="translate(${flight?Math.sin(p*Math.PI)*26:0} ${flight?-Math.sin(p*Math.PI)*35:0}) rotate(${flight?p*396:0} 60 60) translate(60 0) scale(${sx} 1) translate(-60 0)">${art.body}${o.armed?art.hands.join(''):''}</g></svg>`;
}
function renderJoanaCombat(o){
 const data=o.combat,action=o.action,t=Math.max(0,o.time||0),view=o.view==='back'?'B':'F',form=o.state==='monster'?'deep':'base';
 if(['spin','eject'].includes(action))return spin(o);
 const clip=data.clips[form+'/'+view+'/'+action]||data.clips['base/'+view+'/'+action];
 if(!clip)return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-42 -45 204 183" data-combat="${action}" data-unavailable="true"><text x="60" y="50" text-anchor="middle" fill="#bbc6d5" font-family="sans-serif" font-size="7">Não exportado nesta vista.</text><text x="60" y="63" text-anchor="middle" fill="#bbc6d5" font-family="sans-serif" font-size="6">Troque frente / costas.</text></svg>`;
 const duration=o.actionDuration||clip.duration+(clip.loop?0:.45),phase=clip.loop?(t%clip.duration)/clip.duration:Math.min(1,(t%duration)/clip.duration),frameAt=phase*(clip.frames.length-1),fi=Math.floor(frameAt),f=clip.frames[fi],next=clip.frames[Math.min(fi+1,clip.frames.length-1)],pose={},strike=/^(jab|cross|hook|body|upper)_/.test(action);
 for(const inst of f.instances)if(!pose[inst[8]]){const b=next.instances.find(x=>x[8]===inst[8])||inst;pose[inst[8]]=blend(inst.slice(1,7),b.slice(1,7),frameAt-fi);}
 // The game's root already supplies the punch lean. The laboratory pose is a second,
 // independently keyed lean, not an additional joint animation for this redesigned body.
 if(strike)for(const slot of Object.keys(pose))if(!/^leg/.test(slot))pose[slot]=I;
 const mech=action==='signature'&&skinId===3?mechSignature(phase,view==='B'):null;
 if(mech)for(const slot of Object.keys(pose))pose[slot]=/^leg/.test(slot)?I:mech.recoil;
 const expression=action.startsWith('hit')?'pain':action==='groggy'?'game_over':mech?(mech.shock>.1?'pain':'normal'):action==='signature'&&phase>=.4&&phase<.7?'angry':o.expression||'normal',turn=['hit1','hit2'].includes(action)?Math.sin(Math.PI*phase)*.8:0,art=parts({...o,expression,time:t,animate:false,mouthOpen:skinId===1&&action==='signature'&&phase>=.4&&phase<.7,drinking:action==='signature'&&phase>=.4&&phase<.7},pose,turn);
 const rest=data.trajectories[view+'_idle'][0],T=data.trajectories[view+'_'+action],tr=T?(strike?punchTrack(T,phase,rest):sample(T,phase)):rest,unit=rest[2]/120,vunit=rest[3]/120,A=tr[2]/rest[2],D=tr[3]/rest[3],screen=view==='F'?-1:1;
 let skew=tr[4];
 if(strike)skew*=.65;
 const C=screen*skew*D,root=[A,0,C,D,60+screen*(tr[0]-rest[0])/unit-60*A-120*C,120+(tr[1]-rest[1])/unit-120*D];
 const anchors=skinId===1?[[26,68],[o.state==='monster'?94:92,o.state==='monster'?66:68]]:[[24+(skinId>1?1:0),64],[94,66]];
 let back='',front='',signatureFx=o.armed&&mech?mech.fx:'',sigStage=mech?mech.stage:'';
 if(o.armed)for(let i=0;i<2;i++){
  const [ax,ay]=anchors[i];let m=I,G=tr[5+i],R=rest[5+i],artHand=art.hands[i],handBehind=false;
  if(mech){m=mech.hands[i];}
  else if(action==='signature'&&i===1){
   const mouthBase=skinId===0?[58,41]:[60,o.state==='monster'&&skinId===1?45:39],h=pose.head||I,mouth=[h[0]*mouthBase[0]+h[2]*mouthBase[1]+h[4],h[1]*mouthBase[0]+h[3]*mouthBase[1]+h[5]];
   const sig=signature(phase,[ax,ay],view==='B',mouth,{beer:skinId===1});m=sig.m;sigStage=sig.stage;handBehind=sig.behind;artHand=closePincers(artHand,sig.close);
   const humanMug=skinId===1&&o.state!=='monster';
   if(humanMug){
    // Keep the hook stowed for the entire signature loop, including between mugs.
    const handOnly=nodes(inside(artHand)).find(s=>s.includes('data-part="human-hook-hand"'))||'';
    artHand=`<g data-part="fishing-hook" data-hook-stowed="true">${handOnly}</g>`;
   }
   // Draw the bottle over the palm, then repeat the jaw edges over the glass: a real grip.
   if(sig.held){
    const held=gripBottle(artHand,sig.rum,m);artHand=held.svg;if(!held.found){if(handBehind)back+=wrap(sig.rum,root,'data-rum="behind-body"');else if(humanMug)front+=wrap(sig.rum,root);else signatureFx=sig.rum;}
   }else signatureFx=sig.rum;
  }else if(action==='signature'){
   const slot=i?'gR':'gL',first=clip.frames[0].instances.find(x=>x[8]===slot)?.slice(1,7)||I,origin=[first[0]*60+first[2]*60+first[4],first[1]*60+first[3]*60+first[5]],shift=[1,0,0,1,ax-origin[0],ay-origin[1]];
   m=mul(shift,mul(mul(pose[slot]||first,inverse(first)),inverse(shift)));
   const q=Math.min(1,Math.max(0,(phase-.88)/.12)),e=q*q*(3-2*q);m=m.map((v,j)=>v+(I[j]-v)*e);
  }else{
   // Bind by equipment identity, not screen side: front/back tracks mirror hook and claw.
   const dx=screen*(G[0]-R[0])/unit,dy=(G[1]-G[2]/2-(R[1]-R[2]/2))/vunit,s=G[2]/R[2],r=screen*G[3]*180/Math.PI,angle=r*Math.PI/180,c=Math.cos(angle)*s,q=Math.sin(angle)*s;
   m=[c,q,-q,c,ax+dx-c*ax+q*ay,ay+dy-q*ax-c*ay];
   if(skinId===1&&i===0&&s>1){
    const open=artHand.match(/^<g\b[^>]*>/)?.[0]||'<g>',foreshorten=[1,0,0,1/s,0,ay*(1-1/s)];
    artHand=open+nodes(inside(artHand)).map((piece,j)=>j<4?wrap(piece,foreshorten,'data-harpoon-perspective="foreshortened"'):piece).join('')+'</g>';
   }
  }
  if(i===0&&o.specialHookThrown){
   const palm=skinId===1&&o.state!=='monster'?`<g data-native-empty-hand="harpoon-released">${nodes(inside(artHand)).slice(-3).join('')}</g>`:'';
   const metal=skinId===3?'#c7bda7':skinId===2?'#7cc0ca':'#c2d1ce';
   artHand=palm||`<g data-native-wrist-socket="hook-released"><path d="M ${ax-6} ${ay+8} H ${ax+6} V ${ay-2} Q ${ax} ${ay-6} ${ax-6} ${ay-2} Z" fill="${metal}" stroke="#172027" stroke-width="1.9"/><ellipse cx="${ax}" cy="${ay-2}" rx="6" ry="2.7" fill="#334448" stroke="#172027" stroke-width="1.7"/></g>`;
  }
  if(i===1&&o.specialClaw!==undefined)artHand=skinId===1&&o.state!=='monster'?`<g data-hook-strike="rigid" data-hook-deformation="none">${artHand}</g>`:`<g data-native-claw-stage="${n(o.specialNativeClaw??o.specialClaw)}" data-jaw-closure="${n(o.specialClaw)}">${closePincers(artHand,o.specialClaw)}</g>`;
  const hand=wrap(artHand,m,`data-joana-hand="${i?'right':'left'}" data-equipment="${i?'claw-side':'hook-side'}" data-hand-center="${n(m[0]*ax+m[2]*ay+m[4])} ${n(m[1]*ax+m[3]*ay+m[5])}"`);
  // KTRACK hands are already in screen space, so do not apply the body root twice.
  if(action==='signature'){if(handBehind)back+=wrap(hand,root,'data-retrieval="behind-body"');else front+=wrap(hand,root);}else if((strike&&view==='F')||G[5]>=.5)front+=hand;else back+=hand;
 }
 const growth=o.specialBackAdornment||'',armor=o.specialBodyOverlay||'';
 // Lower dread tips tuck beneath the pauldrons/backplate, as in the native
 // armor overlay. Only the face, hat and eye remain in front of the collar.
 // From the rear, preserve head-before-dreads: moving the rear head in front
 // would erase the hair even at zero armor opacity.
 const headPart=s=>new RegExp('^<g\\b[^>]*data-joana-part="(?:'+(view==='F'?'head|':'')+'bandana|stalk)"').test(s),bodyPieces=armor?nodes(art.body):[],assembled=armor?bodyPieces.filter(s=>!headPart(s)).join('')+armor+bodyPieces.filter(headPart).join(''):art.body;
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${action.startsWith('upper')?'-70 -130 260 275':'-48 -48 216 190'}" data-combat="${action}" data-frame="${fi}" data-form="${form}" data-signature-stage="${sigStage}">${art.defs}<ellipse cx="60" cy="115" rx="38" ry="4" fill="#000" opacity=".18"/>${back}${view==='F'?wrap(growth,root):''}${wrap(assembled+(view==='B'?growth:''),root,`opacity="${lerp(f.opacity,next.opacity,frameAt-fi)}"`)}${front}${wrap(signatureFx,root)}</svg>`;
}
return o=>o.exportSpecialParts?parts(o,{},o.exportTurn||0):o.action&&o.combat?renderJoanaCombat(o):drawJoana(o);
})();const skillCharacter="joana";const skillSkinId="abismo";const skillSkinIndex=2;
// Recorded engine geometry, kept separate from character art and loaded on demand.
function spRecordedEffects(o,s,t,cfg,capture){
 const pack=o.nativeEffects;
 if(!pack?.hasEffects||!capture)return null;
 const view=o.view==='back'?'back':'front',frames=pack.views[o.mode+'/'+view]||pack.views[view];
 if(!frames?.length)return null;
 // The game's very high beam endpoint falls behind the near plane in the
 // frontal camera. Keep the documented camera repair instead of an empty cast.
 if(s.key==='batalhator_orbital_apocalypse'&&pack.hasEffectsByView?.[view]===false)return null;
 const frame=frames[Math.min(frames.length-1,Math.floor(t*60+1e-7))],rest=capture.rest.screen;
 const mirror=['jack','frog'].includes(skillCharacter)?1:Math.sign((cfg.anchors[1][0]-cfg.anchors[0][0])*(capture.rest.hands[1][0]-capture.rest.hands[0][0]))||1;
 const floor=skillCharacter==='lady'?350:skillCharacter==='vivaldi'?145:skillCharacter==='valeria'||skillCharacter==='marcov'?130:skillCharacter==='batalhator'?cfg.h-6:115;
 const a=cfg.w/rest[2]*mirror,d=cfg.h/rest[3],matrix=[a,0,0,d,cfg.w/2-a*rest[0],floor-d*rest[1]],themed=skillSkinIndex>0;
 let anchorOffset=[0,0];
 if(s.key==='frog_tongue'){
  const gradients=[...frame.join('').matchAll(/<linearGradient\b[^>]*x1='([-\d.]+)' y1='([-\d.]+)'/g)],g=gradients.at(-1),m=o.specialBodyMatrix;
  if(g){const source=[a*+g[1]+matrix[4],d*+g[2]+matrix[5]],target=[m[0]*60+m[2]*48+m[4],m[1]*60+m[3]*48+m[5]];anchorOffset=target.map((v,i)=>v-source[i]);}
 }
 if(s.key==='frog_fly_catcher'){
  const start=frame.join('').match(/<path d='M([-\d.]+),([-\d.]+)Q[^']*'[^>]*stroke='#881337'/),m=o.specialBodyMatrix;
  if(start){const source=[a*+start[1]+matrix[4],d*+start[2]+matrix[5]],target=[m[0]*60+m[2]*48+m[4],m[1]*60+m[3]*48+m[5]];anchorOffset=target.map((v,i)=>v-source[i]);}
 }
 if(s.key==='joana_hook_pull'&&capture.frame.state===15&&1-capture.frame.timer/capture.frame.max>=.24&&1-capture.frame.timer/capture.frame.max<.88){
  const ellipse=(frame[1]+frame[3]).match(/<path d='(M[^']+)'/),d=ellipse?.[1],ends=d?[...d.matchAll(/(?:M|C)([-\d.]+),([-\d.]+)(?: [-\d.]+,[-\d.]+ ([-\d.]+),([-\d.]+))?/g)].map(m=>m[3]?[+m[3],+m[4]]:[+m[1],+m[2]]):[];
  if(ends.length===5){const source=[a*ends.slice(0,4).reduce((n,p)=>n+p[0],0)/4+matrix[4],matrix[3]*ends.slice(0,4).reduce((n,p)=>n+p[1],0)/4+matrix[5]],target=o.specialHandCenters[0];anchorOffset=target.map((v,i)=>v-source[i]);}
 }
 const palettes={jack:[null,{'#ffee00':'#f4d55e'}, {'#ffee00':'#d9bb65','#f59e0b':'#adac9b','#ea580c':'#8f743e'},{'#ffee00':'#8cf5c8','#ea580c':'#258c83','#f59e0b':'#51d4cc','#e6f5ff':'#92f9e9'}],frog:[null,{'#f472b6':'#d8b256','#ec4899':'#bb9641','#f9a8d4':'#f3dc82','#881337':'#66552a'},{},{'#f472b6':'#28afca','#ec4899':'#168da8','#f9a8d4':'#86eaf1','#881337':'#155c78'}],cremoso:[null,{'#ca8a04':'#668239','#eab308':'#9cbb60','#fef08a':'#d9eab0','#fffde7':'#f5f5d9','#dc2626':'#b8b07e'},{'#ca8a04':'#9c6d4c','#eab308':'#e494a6','#fef08a':'#f3bbca','#fffde7':'#fff0dc'},{'#ca8a04':'#538dac','#eab308':'#82c4dc','#fef08a':'#c5e9ef','#fffde7':'#fff6df','#dc2626':'#d75e9b'}],valeria:[null,{}, {'#ff6600':'#cf272d','#ff3300':'#a61d2e'},{'#ff6600':'#e09e39','#ff3300':'#af682f'}],vivaldi:[null,{'#67e8f9':'#dfbf73','#a5f3fc':'#f5e4b2'},{'#67e8f9':'#ae6dba','#a5f3fc':'#e4bcdb'},{'#67e8f9':'#9de1ea','#a5f3fc':'#dcf7f7'}],joana:[null,{'#f97316':'#d4b64c','#ef4444':'#c3a243'},{'#f97316':'#36bbc0','#ef4444':'#238b9e'},{'#f97316':'#bc853d','#ef4444':'#976330'}],lady:[null,{'#22c55e':'#d9b856'},{'#22c55e':'#ed9d32'},{'#22c55e':'#ad976e'}],marcov:[null,{'#22d3ee':'#d7c6a5'},{},{'#22d3ee':'#a5cbd1'}]}[skillCharacter]?.[skillSkinIndex]||{};
 const adapt=(svg,attached=true)=>{
  if(skillCharacter==='jack')svg=svg.replace(/<g data-native-shield-cell='honey' data-cell-x='([-\d.]+)' data-cell-y='([-\d.]+)' data-cell-radius='([-\d.]+)'>([\s\S]*?)<\/g>/g,(_,x,y,r,art)=>{
   const source=[a*+x+matrix[4],d*+y+matrix[5]],radius=Math.abs(+r)*Math.max(Math.abs(a),d),target=[60+(source[0]-60)*1.35,source[1]+12];
   // Retain all sixteen recorded cells. Move a colliding cell out of the palm
   // instead of deleting it; that preserves the original shield's readable dome.
   const palms=o.specialHandCenters||[];
   for(let pass=0;pass<4;pass++)for(const h of palms){const dx=target[0]-h[0],dy=target[1]-h[1],distance=Math.hypot(dx,dy),min=radius+23;if(distance<min){if(Math.abs(target[0]-60)<14)target[1]=h[1]+Math.sqrt(Math.max(0,min*min-dx*dx));else target[0]=h[0]+Math.sign(target[0]-60)*Math.sqrt(Math.max(0,min*min-dy*dy));}}
   const clearance=Math.min(...palms.map(h=>Math.hypot(target[0]-h[0],target[1]-h[1])))-radius;
   return `<g data-honey-shield-radius="${spN(Math.abs(target[0]-60)+radius)}" data-palm-clearance="${spN(clearance)}" data-native-shield-cell="honey" transform="translate(${spN((target[0]-source[0])/a)} ${spN((target[1]-source[1])/d)})">${art}</g>`;
  });
  if(skillCharacter==='marcov')svg=svg.replace(/<g data-native-satellite='sputnik' data-orbit-x='([-\d.]+)' data-orbit-y='([-\d.]+)' data-orbit-scale='([-\d.]+)' data-orbit-rotation='([-\d.]+)'>([\s\S]*?)<\/g>/g,(full,x,y,scale,rotation)=>o.specialParts?.orbital?`<g data-native-themed-prop="orbital-${skillSkinIndex}" transform="translate(${x} ${y}) rotate(${spN(+rotation*180/Math.PI)}) scale(${spN(+scale*2)})">${o.specialParts.orbital}</g>`:full);
  if(skillCharacter==='frog'&&skillSkinIndex===1)svg=svg.replace(/#(?:f43f5e|e11d48|be123c|fda4af|fb7185)/gi,c=>({'#f43f5e':'#d4ac42','#e11d48':'#b78c30','#be123c':'#5d4b2b','#fda4af':'#efd77b','#fb7185':'#e2bd55'}[c.toLowerCase()]));
  if(skillCharacter==='frog'&&skillSkinIndex===3)svg=svg.replace(/#(?:f43f5e|e11d48|be123c|fda4af|fb7185)/gi,c=>({'#f43f5e':'#37b9d3','#e11d48':'#268ba7','#be123c':'#195876','#fda4af':'#b3eece','#fb7185':'#e8d564'}[c.toLowerCase()]));
  if(skillCharacter==='frog')svg=svg.replace(/<image\b[^>]*data-native-image='frogCrown(?:Poison)?Sprite'[^>]*\/>/g,tag=>{
   const attrs=tag.replace(/^<image\b/,'').replace(/\s(?:data-native-image|href|preserveAspectRatio)='[^']*'/g,'').replace(/\/>$/,'');
   return `<svg ${attrs} viewBox="20 -18 80 48" overflow="visible" data-native-themed-prop="flying-headwear">${o.specialParts?.hats||''}</svg>`;
  });
  svg=svg.replace(/<g data-native-bee-variant='(\d)'>([\s\S]*?)<\/g>/g,(_,si,content)=>+si===skillSkinIndex?`<g data-themed-bee-equipment="${si}">${content}</g>`:'');
  svg=svg.replace(/<g data-native-bee='psx' data-hive-offset='([-\d.]+)'>/g,(_,lift)=>`<g data-native-bee="psx" data-emitter-retarget="hive-socket" transform="translate(0 ${lift})">`);
  if(skillCharacter==='jack'&&skillSkinIndex===3)svg=svg.replace(/<path\b[^>]*data-bee-part='wing'[^>]*\/>/g,'');
  if(skillCharacter==='batalhator'&&themed)svg=svg.replace(/<image\b[^>]*data-native-image='(batalhatorSawLeft|batalhatorSawRight|batalhator3FrontFrames_0)'[^>]*\/>/g,(tag,name)=>{
   const pilot=name.includes('FrontFrames');let attrs=tag.replace(/^<image\b/,'').replace(/\s(?:data-native-image|href|preserveAspectRatio)='[^']*'/g,'').replace(/\/>$/,'');
   const i=name.includes('Right')?1:0,[x,y]=cfg.anchors[i],box=pilot?'0 0 150 146':`${x-34} ${y-34} 68 68`,art=pilot?o.specialParts?.clone:o.specialParts?.hands[i];
   // Dark themed clothing needs an emissive floor; multiplying it by the
   // native blue tint made the alternate pilots nearly invisible. Keep the
   // recorded blinking, opacity, placement and perspective unchanged.
   if(pilot)attrs=attrs.replace(/\sfilter='[^']*'/g,'');
   // Filter in the pilot's 150-unit art space, not the 1-unit projected image
   // viewport. The latter clips the light pass in SVG rasterizers.
   const light=pilot?'<defs><filter id="native-readable-hologram" filterUnits="userSpaceOnUse" x="-30" y="-30" width="210" height="210" color-interpolation-filters="sRGB"><feColorMatrix type="saturate" values="0"/><feComponentTransfer><feFuncR type="linear" slope=".40" intercept=".15"/><feFuncG type="linear" slope=".47" intercept=".38"/><feFuncB type="linear" slope=".42" intercept=".58"/></feComponentTransfer></filter></defs>':'';
   let wake='';
   if(!pilot){const color=skillSkinIndex===1?'#e863c5':skillSkinIndex===2?'#dac394':'#7cddf2';
    wake=`<g data-themed-saw-wake="${skillSkinIndex}" fill="none" stroke="${color}" stroke-width="2" stroke-dasharray="${skillSkinIndex===1?'4 4':skillSkinIndex===2?'9 6':'16 6'}"><circle cx="${x}" cy="${y}" r="29"/><circle cx="${x}" cy="${y}" r="25" stroke-width=".8"/></g>`;
   }
   return `<svg ${attrs} viewBox="${box}" data-native-themed-prop="${pilot?'pilot':'saw'}">${light}${pilot?'<g filter="url(#native-readable-hologram)">':''}${wake}${art||''}${pilot?'</g>':''}</svg>`;
  });
  if(themed&&s.key==='joana_hook_pull')svg=svg.replace(/<image\b[^>]*data-native-image='joanaHookFlyingSprite'[^>]*\/>/g,tag=>{
   const attrs=tag.replace(/^<image\b/,'').replace(/\s(?:data-native-image|href|preserveAspectRatio)='[^']*'/g,'').replace(/\/>$/,'');
   const art=skillSkinIndex===1?'<path d="M -3 32 H 3 V -13 H 9 L 0 -33 L -9 -13 H -3 Z" fill="#78969d" stroke="#14222a" stroke-width="2.2"/><path d="M 0 -29 V -13 H 6" fill="none" stroke="#d5e4db" stroke-width="2"/><path d="M -4 3 H 4 V 20 H -4 Z" fill="#745132" stroke="#14222a" stroke-width="1.8"/>':skillSkinIndex===2?'<path d="M -9 25 V -9 Q -9 -27 7 -27 Q 23 -27 23 -12 Q 23 -1 11 5 L 16 -7 L 6 -3 Q 13 -13 7 -17 Q 1 -21 1 -9 V 25 Z" fill="#8ed9e6" stroke="#153948" stroke-width="2.5"/>':'<path d="M -9 25 V -9 Q -9 -27 7 -27 Q 23 -27 23 -12 Q 23 -1 11 5 L 16 -7 L 6 -3 Q 13 -13 7 -17 Q 1 -21 1 -9 V 25 Z" fill="#c6d3cf" stroke="#252d2c" stroke-width="2.5"/><path d="M -12 15 H 5 V 26 H -12 Z" fill="#bd873e" stroke="#252d2c" stroke-width="2"/><circle cx="-4" cy="20" r="2" fill="#ebe0af"/>';
   return `<svg ${attrs} viewBox="-36 -36 72 72" data-native-themed-prop="${skillSkinIndex===1?'harpoon':'hook'}">${art}</svg>`;
  });
  svg=svg.replace(/href='native-image:([\w-]+)'/g,(_,name)=>`href='${pack.images[name]||''}'`);
  if(themed&&['joana','cremoso'].includes(skillCharacter)){
   const tones=({joana:[null,['#172d3c','#87c9dd','#edf6f0'],['#092c42','#249eae','#c1f3df'],['#252622','#b5884c','#ece1b7']],cremoso:[null,['#1b3925','#8ab05a','#eff0c9'],['#3b2118','#d6909e','#fff0d9'],['#243e66','#80c6df','#f5e8ef']]}[skillCharacter])[skillSkinIndex],id='native-prop-material';
   const tables=[0,1,2].map(i=>tones.map(c=>parseInt(c.slice(1+i*2,3+i*2),16)/255).map(spN).join(' '));
   svg=svg.replace(/<image\b(?![^>]*filter=)[^>]*\/>/g,tag=>tag.replace('/>',` filter="url(#${id})"/>`));
   svg=`<defs><filter id="${id}" color-interpolation-filters="sRGB"><feColorMatrix type="saturate" values="0"/><feComponentTransfer><feFuncR type="table" tableValues="${tables[0]}"/><feFuncG type="table" tableValues="${tables[1]}"/><feFuncB type="table" tableValues="${tables[2]}"/></feComponentTransfer></filter></defs>`+svg;
  }
  if(themed&&skillCharacter==='vivaldi'){
   const notes=[null,{'#5eead4':'#e3bc67','#0f766e':'#8e5c31'},{'#5eead4':'#b779b8','#0f766e':'#6b315f'},{'#5eead4':'#9be6ee','#0f766e':'#367b9e'}][skillSkinIndex];
   svg=svg.replace(/#[0-9a-f]{6}/gi,c=>notes[c.toLowerCase()]||c);
  }
  if(skillCharacter==='jack'&&skillSkinIndex===3)svg=svg.replace(/#(?:ffd21e|fff88c|ffffdc)/gi,c=>({'#ffd21e':'#20bca6','#fff88c':'#74ffe0','#ffffdc':'#dbfff3'}[c.toLowerCase()]));
  if(themed)svg=svg.replace(/#[0-9a-f]{6}/gi,color=>palettes[color.toLowerCase()]||color);
  return `<g transform="translate(${(attached?anchorOffset:[0,0]).map(spN).join(' ')})"><g data-effects-source="native-cpp-capture" transform="matrix(${matrix.map(spN).join(' ')})">${svg}</g></g>`;
 };
 let replacement=frame[4];
 if(replacement&&skillCharacter==='cremoso'&&themed){
  const paths=[...replacement.matchAll(/<path\b[^>]*\/>/g)];
  if(paths.length>=9){const from=paths.at(-9).index,to=paths.at(-5).index;replacement=replacement.slice(0,from)+replacement.slice(to);}
  if(skillSkinIndex===2&&o.mode!=='frozen'&&o.mode!=='revenge'){let i=0;replacement=replacement.replace(/<path\b[^>]*\/>/g,tag=>{const index=i++;return index<=2?tag.replace(/fill='(?!none)[^']*'/,"fill='#70472f'"):index>=8&&index<=15?tag.replace(/fill='(?!none)[^']*'/,"fill='#fff0d6'"):tag;});}
 }
 const extras=replacement&&skillCharacter==='cremoso'&&themed?`<g data-tornado-themed-hat="true" transform="translate(0 10) rotate(${spN(Math.sin(t*18)*8)} 60 15)">${o.specialParts?.hats||''}</g>`:'';
 const result={under:adapt(frame[0],false)+adapt(frame[1]),overlay:adapt(frame[2]),over:adapt(frame[3]),replacement:replacement?adapt(replacement)+extras:'',available:frame.some(Boolean)};
 if(skillCharacter==='marcov'&&s.key==='marcov_orbital_laser'&&view==='back')return {...result,under:`<g data-sputnik-depth="behind-marcov">${result.under}${result.overlay}${result.over}</g>`,overlay:'',over:''};
 return result;
}

// Gallery camera adaptation, not a replacement for the game's world-space VFX.
// Timing and motifs follow app/vfx/lab_*.cpp; approved skins override obsolete guide proposals.
function axialSpecialEffects(s,p,t,o,centers,cfg){
 if(p>=1)return {under:'',over:''};
 const back=o.view==='back',id=skillCharacter,si=skillSkinIndex,key=s.key;
 const ink='#172027',pal={frog:['#e990a5','#d9ae45','#bf883b','#36c7d0'],joana:['#e89673','#c8e3e4','#38b9da','#c18b48'],cremoso:['#e7cc93','#83b44c','#e992ab','#66c8e8'],valeria:['#f26927','#efb53b','#da333d','#b1a697'],vivaldi:['#7de3cd','#e7b76a','#e3476d','#9be2f0'],lady:['#72be6a','#efc54e','#f09230','#d1ad73'],marcov:['#64cced','#b58b61','#76c6ef','#e9d7b4'],batalhator:['#a7bac7','#e45dd0','#c9954f','#80c8ee']}[id],color=pal?.[si]||'#e8d48d';
 const life=spEase(p/.07)*(1-spEase((p-.87)/.13)),pulse=(start,width=.22)=>Math.sin(spClamp((p-start)/width)*Math.PI),window=(start,end)=>spEase((p-start)/.045)*(1-spEase((p-end)/.07));
 const C=centers.map(([x,y])=>[x/cfg.w*120,y/cfg.h*120]),bodyY=o.specialBodyY/cfg.h*120||0;
 const project=(x,y,z=0)=>{const k=260/(260+(back?1:-1)*z);return [60+(x-60)*k,72+(y-72)*k,k];};
 let under='',over='';
 const put=(svg,z=0,opacity=1)=>{opacity*=1-spEase((p-.90)/.10);if(opacity<=.001)return;const q=`<g opacity="${spN(spClamp(opacity))}" data-fx-z="${spN(z)}">${svg}</g>`;if(z===0||(z>0)!==back)over+=q;else under+=q;};
 const path=(d,fill='none',stroke=color,w=1.5)=>`<path d="${d}" fill="${fill}" stroke="${stroke}" stroke-width="${spN(w)}" stroke-linecap="round" stroke-linejoin="round"/>`;
 const circle=(x,y,r,c=color)=>`<circle cx="${spN(x)}" cy="${spN(y)}" r="${spN(Math.max(0,r))}" fill="${c}"/>`;
 const ell=(x,y,rx,ry,c=color,stroke='none',w=1)=>`<ellipse cx="${spN(x)}" cy="${spN(y)}" rx="${spN(Math.max(0,rx))}" ry="${spN(Math.max(0,ry))}" fill="${c}" stroke="${stroke}" stroke-width="${w}"/>`;
 const at=(x,y,z,art,scale=1,rotation=0)=>{const q=project(x,y,z);return `<g transform="translate(${spN(q[0])} ${spN(q[1])}) scale(${spN(q[2]*scale)}) rotate(${spN(rotation)})">${art}</g>`;};
 const ring=(x,y,z,r,c=color,a=1)=>put(at(x,y,z,ell(0,0,r,r*.27,'none',c,1.7)),z,a);
 const shards=(x,y,z,start,c=color,n=12,power=1)=>{const u=spClamp((p-start)/.27),a=Math.sin(u*Math.PI);for(let i=0;i<n;i++){const ang=i*2.399,r=u*(10+i%5*4)*power;put(at(x+Math.cos(ang)*r,y+Math.sin(ang)*r-u*9,z+Math.sin(i)*8,path('M -1 -3 L 2 0 L 0 3 L -2 0 Z',c,ink,.5),.7+(i%3)*.25,i*39+u*180),z,a);}};
 const cloud=(x,y,z,start,c='#a5aaa7',n=12)=>{for(let i=0;i<n;i++){const u=spClamp((p-start-i*.012)/.40),a=Math.sin(u*Math.PI)*.42;if(a>.001)put(at(x+Math.cos(i*2.399)*(3+u*23),y+Math.sin(i*2.399)*(3+u*12)-u*14,z+u*80,circle(0,0,2+u*(4+i%3),c)),z+u*80,a);}};
 const hex=(r,c=color)=>path(Array.from({length:6},(_,i)=>`${i?'L':'M'} ${spN(Math.cos(i*Math.PI/3)*r)} ${spN(Math.sin(i*Math.PI/3)*r)}`).join(' ')+' Z',c,ink,.8);
 const flower=(r,c=color)=>Array.from({length:8},(_,i)=>`<g transform="rotate(${i*45})">${ell(0,-r*.65,r*.35,r*.65,c,ink,.5)}</g>`).join('')+circle(0,0,r*.3,'#f4cc55');
 const crystals=(z,start,c=color)=>{const grow=spEase((p-start)/.10)*(1-spEase((p-.78)/.15));for(let i=0;i<7;i++){const x=60+(i-3)*6,h=(17+(i*7%19))*grow;put(at(x,119,z,path(`M -4 0 L -3 ${-h*.66} L 0 ${-h} L 5 ${-h*.68} L 4 0 Z`,c,ink,1)+path(`M 0 0 V ${-h} M -3 ${-h*.66} L 0 ${-h*.45} L 5 ${-h*.68}`,'none','#edfaff',.65)),z,grow);}if(p>.78)shards(60,96,z,.78,c,16);};
 const thrust=(hand,start,c=color)=>{const u=spEase((p-start)/.20)*(1-spEase((p-.72)/.18)),z=12+u*105,x=hand[0]+(60-hand[0])*u*.85;put(at(x,hand[1],z,path('M 0 -12 L 7 0 L 0 12 L -7 0 Z',c,ink,1.3)+path('M 0 -9 L 3 0 L 0 7 Z','#fff4cf','none'),.3+u*.7),z,window(start,.78));};
 const impact=(z,start,c=color)=>{const u=spClamp((p-start)/.24);ring(60,119,z,5+u*35,c,Math.sin(u*Math.PI));shards(60,111,z,start,c,16);};
 const orbit=(count,r,y,art,alpha=life)=>{for(let i=0;i<count;i++){const a=t*2.2+i*Math.PI*2/count,z=Math.sin(a)*r;put(at(60+Math.cos(a)*r,y+Math.sin(a*2)*3,z,art,i%2?.85:1,a*180/Math.PI),z,alpha);}};
 if(id==='frog'){
  const aperture=typeof o.specialMouthStage==='number'?[0,.22,.43,.70,1,.65,.25][o.specialMouthStage]||0:1;
  const mouth=[60,43+8*aperture+bodyY];
  if(/tongue|fly_catcher/.test(key)){
   const catchFly=/fly_catcher/.test(key),u=spClamp((p-.22)/.52),ext=u<.16?Math.sin(u/.16*Math.PI/2):u>.84?Math.cos((u-.84)/.16*Math.PI/2):1,active=p>.22&&p<.74;
   if(active){
    // Original tongue sweeps in X while reaching forward in Z, not at a side-on target.
    const ease=(1-Math.cos(u*Math.PI))/2,tip=[60+(back?-1:1)*(-85+ease*170)*ext,51+bodyY+Math.sin(u*Math.PI)*11-Math.cos(u*Math.PI*2)*5,10+Math.sin(u*Math.PI)*130*ext];
    if(catchFly){tip[0]=60+Math.sin(t*9)*23*ext;tip[1]=51+bodyY-28*ext;tip[2]=25+70*ext;}
    const pts=Array.from({length:17},(_,i)=>{const q=i/16;return project(mouth[0]+(tip[0]-mouth[0])*q,mouth[1]+(tip[1]-mouth[1])*q+Math.sin(q*Math.PI)*7,tip[2]*q);});
    const d=pts.map((q,i)=>`${i?'L':'M'} ${spN(q[0])} ${spN(q[1])}`).join(' '),tongue=si===1?'#e6ba46':si===3?'#208bcb':'#ed90a2';
    put(`<g data-special-tongue="${spN(ext)}">${path(d,'none',ink,7)}${path(d,'none',tongue,4.8)}${si===1?path(d,'none','#35332a',3).replace('stroke-linecap="round"','stroke-dasharray="3 4"'):path(d,'none',si===3?'#f2d653':'#fbc2c9',1.2)}</g>`,20);
   }
   if(catchFly)for(let i=0;i<4;i++){const a=1-spEase((p-(.34+i*.06))/.06),x=60+Math.sin(t*7+i*2)*25,y=23+Math.cos(t*6+i)*9+bodyY;put(at(x,y,55,ell(-2,-2,2.3,1.5,'#dbe6df')+ell(2,-2,2.3,1.5,'#dbe6df')+circle(0,0,1.7,'#27362c')),55,a*life);}
  }else if(/royal_leap/.test(key)){
   impact(0,.06,'#b6a778');if(p>=.84){impact(0,.84,si===2?'#dfcba6':color);for(let i=0;i<8;i++){const a=i*Math.PI/4,r=spEase((p-.84)/.13)*38;put(path(`M 60 119 L ${spN(60+Math.cos(a)*r*.65)} ${spN(119+Math.sin(a)*r*.15)} l ${spN(Math.cos(a+.25)*r*.4)} ${spN(Math.sin(a+.25)*r*.1)}`,'none','#51442c',1.4),0,life);}}
  }else if(/mud_slime/.test(key)){
   const u=spClamp((p-.25)/.39),q=project(60,51+bodyY+u*61-24*Math.sin(u*Math.PI),u*125);put(circle(q[0],q[1],(5+u)*q[2],si===3?'#25bba7':'#78603b'),u*125,window(.25,.66));if(p>.64){ring(60,119,125,25*spEase((p-.64)/.1),si===3?'#269d91':'#78603b',life);shards(60,115,125,.64,color,12);}
  }else if(/swamp_spout/.test(key)){
   const h=140*(p<.60?spEase((p-.35)/.25):1-spEase((p-.60)/.25)),z=105,c=si===1?'#7b9186':si===3?'#29c7c5':'#66b7b8';
   ring(60,119,z,12+10*spEase(p/.35),c,life);if(si===1)put(at(60,117,z,ell(0,0,14,4,'#4a5053',ink,1.5),1,-35*spEase((p-.35)/.12)),z,window(.15,.85));
   put(at(60,119,z,path(`M -10 0 Q -16 ${-h*.6} -5 ${-h} Q 0 ${-h-8} 5 ${-h} Q 17 ${-h*.5} 11 0 Z`,c,ink,1.5)+path(`M -4 -1 Q 1 ${-h*.55} -1 ${-h*.92}`,'none','#cefff2',2)),z,window(.35,.84));shards(60,119-h,z,.4,si===3?'#e8cb54':'#b4eae0',15);
  }else{cloud(60,65+bodyY,0,.15,si===1?'#89918c':'#ac74bf',18);orbit(12,39,65+bodyY,si===3?path('M -2 -4 V 4 M 2 -4 V 4 M -3 0 H 3','none','#d3ed74',1):circle(0,0,1.7,'#ad7cc9'));}
 }else if(id==='joana'){
  const claw=C[1],hook=C[0],metal=si===3?'#d3c5a8':si===2?'#68c8d8':'#c5d7d5';
  if(/claw_crush/.test(key)){const a=pulse(.52,.27);put(at(claw[0],claw[1],45,path('M -16 -16 Q 0 -4 16 16 M 16 -16 Q 0 -4 -16 16','none',color,3.5)+path('M -10 0 H 10 M 0 -10 V 10','none','#f5dbb0',1.5),.4+a*.8),45,a);shards(claw[0],claw[1],65,.52,color,13);}
  else if(/hook_pull/.test(key)){
   const reach=spEase((p-.24)/.20)*(1-spEase((p-.66)/.22)),z=120*reach,orbit=p>.44&&p<.66?Math.sin((p-.44)/.22*Math.PI):0,x=hook[0]+(60-hook[0])*reach+Math.cos(t*18)*12*orbit,y=hook[1]+(65-hook[1])*reach+Math.sin(t*18)*8*orbit,q=project(x,y,z),from=project(hook[0],hook[1],0),dx=q[0]-from[0],dy=q[1]-from[1];
   const links=Array.from({length:12},(_,i)=>ell(from[0]+dx*i/12,from[1]+dy*i/12,1.2,i%2?1.8:.7,'none',metal,.8)).join('');
   put(links+at(x,y,z,path('M 0 -6 V 3 Q 0 10 6 7 Q 10 4 7 1','none',ink,4)+path('M 0 -6 V 3 Q 0 10 6 7 Q 10 4 7 1','none',metal,2.4)),z,window(.24,.88));shards(hook[0],hook[1],10,.88,'#efca6d',8);
  }else if(/crustacean_shell/.test(key)){
   for(let i=0;i<14;i++){const a=i*2.399,x=60+Math.cos(a)*(22+i%3*3),y=72+Math.sin(a)*24+bodyY,art=si===3?hex(4.5,'#aa7d44')+circle(0,0,.8,'#eee0a5'):si===2?flower(3,'#37bcbf'):hex(3.5,'#d7dacf')+circle(0,0,1.2,'#6f8c88');put(at(x,y,back?-3:3,art,spEase((p-.15)/.35)),back?-3:3,life);}
   if(si===3)cloud(60,73+bodyY,-5,.43,'#bbc8c6',7);
  }else if(/coral_spout/.test(key)){
   if(si===1)crystals(110,.30,'#c1e4e5');
   else{const grow=spEase((p-.30)/.28)*(1-spEase((p-.80)/.20)),z=110;for(let i=0;i<5;i++){const x=60+(i-2)*8,h=(22+i%3*9)*grow;
    const d=si===3?`M -2 0 V ${-h} H 3 M 2 ${-h*.55} H 7 V ${-h*.85}`:`M 0 0 Q -2 ${-h*.5} 0 ${-h} M 0 ${-h*.4} Q -10 ${-h*.5} -8 ${-h*.82} M 0 ${-h*.65} Q 9 ${-h*.66} 8 ${-h*.95}`;
    put(at(x,119,z,path(d,'none',ink,5)+path(d,'none',si===2?'#255d7a':color,3)+circle(0,-h,1.6,si===2?'#58d6e7':'#efd3ab')),z,grow);
   }if(si===3)cloud(60,98,z,.4,'#bec9c1',10);else shards(60,114,z,.32,si===2?'#61c5d3':'#a9dbdb',12);}
  }
  else if(/anchor_slam/.test(key)){
   const a=window(.12,.69),lift=spEase((p-.12)/.18)*(1-spEase((p-.47)/.19)),x=hook[0],y=hook[1]-26*lift,z=30+65*spEase((p-.44)/.20);
   put(at(x,y,z,circle(0,-8,3,metal)+path('M 0 -5 V 13 M -7 0 H 7 M -12 6 Q 0 23 12 6 M -12 6 l 1 7 M 12 6 l -1 7','none',ink,4.5)+path('M 0 -5 V 13 M -7 0 H 7 M -12 6 Q 0 23 12 6','none',metal,2.5)),z,a);impact(15,.35,color);
   for(let i=0;i<4;i++){const u=spClamp((p-.35-i*.04)/.40);ring(60,117,15+u*110,12+u*25,si===3?'#c1d0c6':'#8fcbd4',Math.sin(u*Math.PI));}
  }else{for(let i=0;i<5;i++){const a=i*Math.PI*2/5,z=Math.sin(a)*25;put(at(60+Math.cos(a)*30,103, z,flower(5,color),spEase((p-.12)/.25)),z,life);}orbit(9,33,70+bodyY,circle(0,0,1.3,si===3?'#e4d5a7':'#91e6d2'));}
 }else if(id==='cremoso'){
  const flavor=si===1?'#4e8c48':si===2?'#b9667e':si===3?'#60c9e8':'#9c5138',hand=C[1];
  if(/syrup_blast/.test(key)){
   for(let i=0;i<18;i++){const u=spClamp((p-.35-i*.008)/.30),z=u*125,H=C[i%2];put(at(H[0]+(60-H[0])*u+Math.sin(i*2.4)*u*8,H[1]+u*12-15*Math.sin(u*Math.PI),z,ell(0,0,2.5,4,flavor,ink,.6)),z,Math.sin(u*Math.PI));}impact(125,.65,flavor);
  }else if(/vanilla_whip/.test(key)){
   const alpha=window(.12,.83);for(let j=0;j<7;j++)for(let i=0;i<32;i++){const r=16+j*2,a=t*9+j*.75+i*Math.PI/16,b=a+Math.PI/16,za=Math.sin(a)*r,zb=Math.sin(b)*r,A=project(60+Math.cos(a)*r,105-j*9,za),B=project(60+Math.cos(b)*r,105-j*9,zb);put(path(`M ${spN(A[0])} ${spN(A[1])} L ${spN(B[0])} ${spN(B[1])}`,'none',j%2?color:si===2?'#f2cc98':'#f2e5cc',2.5),(za+zb)/2,alpha);}
  }else if(/melt_mode/.test(key)){ring(60,118,0,34*spEase((p-.14)/.30),flavor,life);for(let i=0;i<3;i++)ring(60+i*2,118+i,0,(23-i*5)*spEase((p-.17)/.30),si===2?['#d288a3','#f3d39c','#71422e'][i]:flavor,life*.6);}
  else if(/puddle_trap/.test(key)){const z=7;put(at(60,119,z,ell(0,0,29*spEase((p-.15)/.20),5,flavor,ink,1.2)),z,life);for(let i=0;i<9;i++){const u=spClamp((p-.20-i*.035)/.27);put(at(60+Math.cos(i*2.399)*20,116-u*8,z,ell(0,0,1+u*2,1+u*2,flavor,'#f3dfb8',.6)),z,Math.sin(u*Math.PI));}}
  else if(/cone_punch/.test(key)){const u=window(.17,.77),z=105*spEase((p-.17)/.25),art=path('M 0 -14 L 9 8 L -9 8 Z','#d5a45e',ink,1.4)+path('M -5 -1 L 4 7 M -3 -7 L 6 2 M 5 -1 L -4 7 M 3 -7 L -6 2','none','#916333',.8);put(at(hand[0],hand[1],z,art,.35+.65*u),z,u);}
  else{const u=spClamp((p-.48)/.28),z=12+125*u,y=65+bodyY,art=si===1?path('M -5 1 Q -9 -4 -3 -8 Q 0 -2 2 -9 Q 7 -7 6 -2 Q 9 4 3 7 Z','#cf6c85',ink,1):si===2?circle(0,0,6,'#67432e')+path('M -3 -3 L 1 -5 L 3 -2 Z','#e2c778','none'):circle(0,0,6,'#cc465b')+path('M 0 -5 Q -2 -12 6 -13','none','#547d40',1.5);put(at(60,y,z,art,1,t*170),z,window(.48,.78));shards(60,65,137,.76,si===1?'#ad5263':'#d64b67',20);}
 }else if(id==='valeria'){
  const fire=si===3?'#e2b066':si===2?'#f0653d':'#fa9e32',hand=C[1];
  if(/magma_punch/.test(key)){put(at(hand[0],hand[1],35,circle(0,0,9,fire)+circle(0,0,4,'#fff0a4')),35,pulse(.18,.46));shards(hand[0],hand[1],70,.40,fire,20);}
  else if(/volcanic_erupt/.test(key)){
   const h=120*pulse(.55,.38),z=100;put(at(60,119,z,path(`M -12 0 L -9 ${-h*.7} L -3 ${-h} L 2 ${-h*.8} L 8 ${-h*.94} L 13 0 Z`,'#cf4d31',ink,1.8)+path(`M -4 0 L 0 ${-h*.86} L 4 0 Z`,'#ffd564','none')),z,window(.55,.93));impact(z,.55,fire);shards(60,119-h,z,.60,fire,18);cloud(60,115,z,.13,'#83716b',6);
  }else if(/heatwave_guard/.test(key)){
   orbit(18,40,64+bodyY,si===2?path('M -3 -4 Q 4 -5 4 0 Q 3 4 -2 3 Q -4 2 -3 -4 Z','#bd3843',ink,.6):path('M -2 3 L -3 -2 L 1 -5 L 4 1 Z',si===3?'#625c5b':'#bd7339',ink,.6));ring(60,116,0,38,fire,life);
  }else if(/lava_kick/.test(key)){const grow=spEase((p-.38)/.13)*(1-spEase((p-.79)/.18)),h=55*grow;put(at(60,119,110,path(`M -9 0 L -4 ${-h*.7} L 1 ${-h} L 8 ${-h*.47} L 10 0 Z`,'#38323a',ink,1.8)+path(`M 0 -2 L 2 ${-h*.25} L -1 ${-h*.45} L 2 ${-h*.80}`,'none',fire,1.5)),110,grow);impact(110,.38,fire);}
  else if(/ash_cloud/.test(key)){cloud(hand[0],hand[1],15,.14,si===2?'#785f62':si===3?'#6c6b6a':'#747071',20);}
  else{
   // Wings are behind the torso in front view, in front of its back in rear view.
   const lift=spEase((p-.12)/.25);let wings='';
   for(const side of [-1,1])for(let i=0;i<7;i++){const q=i/6,x=60+side*(19+36*q)*lift,y=70+bodyY-(38-24*q)*lift;
    wings+=path(`M 60 ${80+bodyY} Q ${60+side*20*lift} ${38+bodyY} ${x} ${y} Q ${x-side*4} ${y+14} ${60+side*14} ${82+bodyY} Z`,i%2?'#e97832':'#dd5930',ink,1.2);
    wings+=path(`M 60 ${78+bodyY} Q ${60+side*22*lift} ${55+bodyY} ${x-side*5} ${y+9}`,'none',fire,1.5);
   }
   put(wings,-12,life);shards(60,65+bodyY,-15,.35,'#ffd05c',24);
  }
 }else if(id==='vivaldi'){
  const note=(r,c=color)=>ell(-r*.2,0,r,r*.65,c)+path(`M ${r*.65} 0 V ${-r*3} Q ${r*2.4} ${-r*2.8} ${r*2} ${-r*1.4}`,'none',c,1.1);
  if(/cadence_staccato/.test(key)){
   for(let j=0;j<5;j++){const a=project(60+(j-2)*4,64+bodyY,0),b=project(60+(j-2)*4,64,125);put(path(`M ${spN(a[0])} ${spN(a[1])} L ${spN(b[0])} ${spN(b[1])}`,'none',color,.55),70,life*.4);}
   for(let i=0;i<5;i++){const u=spClamp((p-.20-i*.15)/.22),z=125*u;put(at(60+Math.sin(i*2.4)*5,64+bodyY*(1-u),z,note(2+i*.35,si===2?'#e7476b':color)),z,Math.sin(u*Math.PI));}
  }else if(/winter_allegro/.test(key)){crystals(110,.45,si===2?'#77738d':si===3?'#a0dce9':'#bbdcec');shards(C[1][0],C[1][1],15,.18,si===2?'#aaa0bc':'#d6f5ff',12);}
  else if(/four_seasons/.test(key)){for(let i=0;i<16;i++){const a=t*2+i*Math.PI/8,z=Math.sin(a)*37,c=['#edb2cb','#f3cd58','#ce8049','#bce8fa'][i%4],art=i%4===3?path('M -3 0 H 3 M 0 -3 V 3 M -2 -2 L 2 2 M 2 -2 L -2 2','none',c,.8):i%4===1?circle(0,0,1.7,c):ell(0,0,2,4,c);put(at(60+Math.cos(a)*37,65+bodyY+Math.sin(a*2)*20,z,art,1,a*180/Math.PI),z,life);}ring(60,112,0,38,color,life);}
  else if(/crescendo_jab/.test(key)){thrust(C[1],.20,color);shards(C[1][0],C[1][1],110,.42,color,14);}
  else if(/autumn_requiem/.test(key)){for(let i=0;i<4;i++){const u=spClamp((p-.14-i*.09)/.36),z=u*125,r=8+u*13;put(at(60,64,z,ell(0,0,r,r,'none',si===2?'#bc589d':'#a886d4',1.6)),z,Math.sin(u*Math.PI));}for(let i=0;i<14;i++){const u=spClamp((p-.4-i*.013)/.4);put(at(60+Math.cos(i*2.399)*24,35+u*65,105,ell(0,0,2.5,4.5,'#ce864d'),1,i*31+t*90),105,Math.sin(u*Math.PI));}}
  else{put(at(60,111,0,flower(28,si===3?'#b4e7f2':'#efb3d0'),spEase((p-.08)/.28)*life),0);orbit(8,32,70+bodyY,note(1.8,si===1?'#dfb77a':'#91cfad'));}
 }else if(id==='lady'){
  if(/nopal_slap/.test(key)){for(let i=0;i<2;i++){const a=pulse(i?.58:.35,.21);put(at(60+(i?4:-4),70+bodyY,85,path(`M ${i?16:-16} -18 Q ${i?-22:22} 0 ${i?12:-12} 19`,'none',color,3),.5+.5*a),85,a);shards(60,70+bodyY,85,i?.58:.35,'#e5d4ad',8);}}
  else if(/lucha_flyer/.test(key)){impact(0,.12,color);if(p>=.82){impact(0,.82,color);shards(60,99,0,.82,si===2?'#ed8c2d':'#efd069',18);}}
  else if(/thorn_veil/.test(key)){const art=si===2?ell(-2,-1,3,4,'#ed963b',ink,.7)+ell(2,-1,3,4,'#ed963b',ink,.7)+path('M 0 -3 V 3','none',ink,.8):path('M 0 -6 L 3 4 L -3 4 Z',si===1?'#f2d679':'#e7dbb3',ink,.7);orbit(18,42,67+bodyY,art);}
  else if(/needle_jab/.test(key)){thrust(C[1],.30,'#e6dbb5');shards(C[1][0],C[1][1],115,.43,color,14);}
  else if(/desert_wind/.test(key)){const z=spEase((p-.4)/.3)*115;for(let j=0;j<7;j++)for(let half=0;half<2;half++){const pts=Array.from({length:13},(_,i)=>{const a=t*9+j*.5+half*Math.PI+i*Math.PI/12,r=8+j*3;return project(60+Math.cos(a)*r,117-j*8,z+Math.sin(a)*r);});put(path(pts.map((q,i)=>`${i?'L':'M'} ${spN(q[0])} ${spN(q[1])}`).join(' '),'none',si===2?'#cb8e55':'#c8ac78',1.8),z+(half?-15:15),life*.7);}}
  else{for(let i=0;i<5;i++){const a=i*Math.PI*2/5,z=Math.sin(a)*27,art=si===2?path('M -2 0 H 2 V 8 H -2 Z','#eee2bd',ink,.5)+path('M 0 -5 Q 3 -2 0 0 Q -3 -2 0 -5 Z','#ecab3e','none'):flower(5,si===3?'#c78d91':'#dc7498');put(at(60+Math.cos(a)*30,103,z,art,spEase((p-.1)/.3)),z,life);}if(!back)for(let i=0;i<6;i++){const u=spClamp((p-.18-i*.07)/.25);put(at(60+(i%2?5:-5),42+bodyY+u*27,5,ell(0,0,1,1.8,'#e2bd55')),5,Math.sin(u*Math.PI));}}
 }else if(id==='marcov'){
  const satellite=si===1?path('M -6 -3 L -3 -7 L 4 -6 L 7 0 L 3 6 L -4 5 Z','#83715c',ink,1)+path('M -2 -3 L 2 0 L -2 3','none','#da9960',1):si===3?path('M -11 -4 H 11 V -1 H -11 Z M -9 4 H 9 V 7 H -9 Z','#a84937',ink,.8)+ell(0,1,3,9,'#ddbd8b',ink,.8)+path(`M -6 ${-8+Math.sin(t*48)*3} H 6`,'none','#f6e7c9',1.4):si===2?circle(0,0,6,'#c6d2db')+path('M -7 -2 H -15 V 3 H -7 M 7 -2 H 15 V 3 H 7','#bbaa62',ink,.7)+circle(0,0,2,'#b4483d'):circle(0,0,5,'#b7d6df')+path('M -4 -4 L -12 -12 M 4 -4 L 12 -12 M -4 4 L -12 12 M 4 4 L 12 12','none','#e0dfc9',1);
  if(/rocket_thrust|kinetic_overdrive/.test(key)){if(/rocket/.test(key)){for(const side of [-1,1]){put(at(60+side*18,88+bodyY,-15,path(`M -4 -5 Q ${side*9} ${13+Math.sin(t*35)*3} 4 -5 Z`,si===1?'#bc9a69':si===3?'#e8e3d8':'#75bfee',ink,1)),-15,window(.08,.74));}shards(60,70+bodyY,100,.5,color,18);}else{put(at(C[1][0],C[1][1],22,circle(0,0,8,color)+circle(0,0,3,'#f8efc9')),22,pulse(.13,.50));shards(C[1][0],C[1][1],100,.4,color,16);}}
  else if(/orbital_laser/.test(key)){
   const sky=72+(-18-72)/project(60,0,105)[2],height=119-sky;
   put(at(60,sky,105,satellite,1.1),105,life);ring(60,119,105,15,color,life);const beam=window(.55,.85);put(at(60,119,105,si===1?path(`M 0 ${-height} L -7 ${-height*.75} L 4 ${-height*.79} L -4 ${-height*.42} L 6 ${-height*.49} L 0 0`,'none','#f4d080',5):path(`M 0 ${-height} V 0`,'none',si===3?'#ed6a4e':color,9)+path(`M 0 ${-height} V 0`,'none','#fcf5df',2)),105,beam);impact(105,.55,color);
  }else if(/plasma_barrier/.test(key)){
   orbit(2,44,59+bodyY,satellite);for(let row=0;row<3;row++)for(let i=0;i<9;i++){const a=i*Math.PI*2/9+row*.18,z=Math.sin(a)*37;put(at(60+Math.cos(a)*42,40+bodyY+row*21,z,hex(7,'none')),z,life*.48);}
  }else if(/exhaust_cloud/.test(key)){cloud(60,85+bodyY,5,.10,si===1?'#a68b67':si===3?'#dedbd2':'#8f9ba1',20);}
  else{for(let i=0;i<15;i++){const u=spClamp((p-.1-i*.028)/.4),a=i*2.399,z=Math.sin(a)*23;put(at(60+Math.cos(a)*28,103-u*48+bodyY,z,si===1?path('M -2 -2 L 2 -1 L 1 2 L -2 1 Z','#bc9d71',ink,.5):ell(0,0,1.7+i%3*.6,1.7+i%3*.6,'none',color,.8)),z,Math.sin(u*Math.PI));}ring(60,109,0,33,color,life);}
 }else if(id==='batalhator'){
  if(/dual_saw/.test(key))for(let i=0;i<2;i++){
   const u=Math.sin(p*Math.PI),z=u*120,hand=C[i],r=si===3?10:12,n=si===3?8:24,art=path(Array.from({length:n},(_,j)=>{const a=j*Math.PI*2/n,rad=j%2?r*.72:r;return `${j?'L':'M'} ${spN(Math.cos(a)*rad)} ${spN(Math.sin(a)*rad)}`;}).join(' ')+' Z',si===1?'#283344':si===2?'#af8b52':si===3?'#69c5e7':'#b4c1c9',ink,1.2)+circle(0,0,4,si===1?'#df60bd':'#d8b964');put(at(hand[0]+(60-hand[0])*u*.35,hand[1],z,art,1,t*1300*(i?-1:1)),z,life);
  }else if(/overclock|emp/.test(key)){for(let i=0;i<3;i++){const u=spClamp((p-.38-i*.05)/.30);ring(60,115,40+u*80,7+u*40,/emp/.test(key)?color:'#ec9649',Math.sin(u*Math.PI));}shards(60,74,70,.4,color,20);}
  else if(/rocket_barrage/.test(key))for(let i=0;i<4;i++){const u=spClamp((p-.12-i*.12)/.25),z=u*125,x=60+(i%2?-21:21)*(1-u),y=66-i%2*8-25*Math.sin(u*Math.PI);put(at(x,y,z,path('M 0 -6 L 4 0 L 3 6 H -3 L -4 0 Z',si===2?'#ad864b':'#bac7d0',ink,1)+path('M -2 5 L 0 12 L 2 5 Z','#efa44b','none')),z,Math.sin(u*Math.PI));shards(60,70,120,.37+i*.12,color,8);}
  else if(/orbital_apocalypse/.test(key)){
   const alpha=window(.40,.85);ring(60,119,110,26,color,life);if(si===3){for(let i=0;i<3;i++)put(at(60+(i-1)*6,119,110,path('M 0 -125 V 0','none',['#f05e56','#eacb5b','#6cb7e7'][i],5)),110,alpha);}else if(si===2)cloud(60,110,110,.40,'#b9b4a4',30);else put(at(60,119,110,path('M 0 -130 V 0','none',si===1?'#dd68bd':'#ef4444',14)+path('M 0 -130 V 0','none','#f8eedb',3)),110,alpha);impact(110,.46,color);
  }else{for(let i=0;i<3;i++){const a=window(.25,.85),z=110,art=o.specialClone?`<g data-hologram="themed-pilot" transform="scale(.35) translate(${- (si===0?60:75)} -146)">${o.specialClone}</g>`:circle(0,0,5,color);put(at(60+(i-1)*20,118,z,art),z,a*(.26+.10*(1+Math.sin(t*24+i))));}}
 }
 return {under:`<g data-special-layer="axial-behind">${under}</g>`,over:`<g data-special-layer="axial-front">${over}</g>`};
}

// The C++ engine owns every frame, perspective change, rope rebound and hand
// depth. Approved front/back artwork is retargeted; its intermediate profiles
// remain adaptations, not newly claimed native drawings.
function koRender(o){
 const phase=skillCharacter==='batalhator'?+o.phase||1:1,sequence=o.combat.ejection[phase].views[o.view==='back'?'back':'front'];
 const duration=o.actionDuration||sequence.frames.length/60,t=Math.max(0,o.time||0)%duration,index=Math.min(sequence.frames.length-1,Math.floor(t*60+1e-7)),F=sequence.frames[index],R=sequence.rest,cfg=spConfig(o),w=cfg.w,h=cfg.h;
 const unitX=w/R.screen[2],unitY=h/R.screen[3],frame=Number(F.sprite.slice(-1));
 // Batalhator has no RxSpin sprites: the original renderer retains its front
 // or back camera artwork and rolls that quad. It never switches to a front
 // drawing while the back-view pilot is being ejected.
 const art=F.valid?angleArt(o,frame):null,origin=art?.origin||(angleNativeSprites(o)?'native-atlas-sprite':skillCharacter==='batalhator'?'approved-native-camera-view':'approved-profile-adaptation');
 let behind='',over='',actor='',overlay='';
 if(F.valid){
  const A=F.screen[2]/R.screen[2]*(F.mirror?-1:1),D=F.screen[3]/R.screen[3],c=Math.cos(F.roll),s=Math.sin(F.roll),cx=w/2+(F.screen[0]-R.screen[0])*unitX,cy=h+(F.screen[1]-R.screen[1])*unitY-F.screen[3]*unitY/2;
  // Rotate in the native projected screen, then map into each skin's drawing
  // units. Rotating directly in a tall SVG stretches Lady's diagonal frames.
  const a=c*A,b=s*A*unitY/unitX,q=-s*D*unitX/unitY,d=c*D;
  const matrix=[a,b,q,d,cx-a*w/2-q*h/2,cy-b*w/2-d*h/2];
  actor=`${art.defs}<g data-native-ko-body="true" transform="matrix(${matrix.map(spN).join(' ')})">${angleSurface(art.body,o,art.origin)}</g>`;
  if(F.overlay)overlay=`<g data-native-ko-overlay="captured" transform="matrix(${[unitX,0,0,unitY,w/2-R.screen[0]*unitX,h-R.screen[1]*unitY].map(spN).join(' ')})">${F.overlay}</g>`;
  if(o.armed)for(let i=0;i<2;i++){
   const G=F.hands[i],B=R.hands[i];if(!G[2]||!B[2])continue;
   // A physical native hand is not necessarily the SVG's screen-left hand.
   const left=B[0]<R.hands[1-i][0],j=(cfg.anchors[0][0]<cfg.anchors[1][0])==left?0:1,[ax,ay]=cfg.anchors[j];
   const x=ax+(G[0]-B[0])*unitX,y=ay+(G[1]-G[2]/2-B[1]+B[2]/2)*unitY,scale=G[2]/B[2];
   const hc=Math.cos(G[3])*scale,hs=Math.sin(G[3])*scale,hb=hs*unitY/unitX,hq=-hs*unitX/unitY,hm=[hc,hb,hq,hc,x-hc*ax-hq*ay,y-hb*ax-hc*ay];
   const hand=`${art.defs}<g data-native-ko-hand="${i}" data-art-hand="${j}" data-native-depth="${G[5]?'front':'behind'}" data-native-hand-angle="${G[3]}" data-native-hand-scale="${spN(scale)}" data-hand-center="${spN(x)} ${spN(y)}" transform="matrix(${hm.map(spN).join(' ')})">${art.hands[j]}</g>`;
   if(G[5])over+=hand;else behind+=hand;
  }
 }
 if(F.star[2]){
  const x=w/2+(F.star[0]-R.screen[0])*unitX,y=h+(F.star[1]-R.screen[1])*unitY,blink=.5+.5*Math.sin(F.engineFrame*.6),rx=(10+8*blink)*unitX,ry=(10+8*blink)*unitY;
  over+=`<path data-native-ko-star="true" d="M${spN(x)} ${spN(y-2*ry)} L${spN(x+.35*rx)} ${spN(y-.35*ry)} L${spN(x+2*rx)} ${spN(y)} L${spN(x+.35*rx)} ${spN(y+.35*ry)} L${spN(x)} ${spN(y+2*ry)} L${spN(x-.35*rx)} ${spN(y+.35*ry)} L${spN(x-2*rx)} ${spN(y)} L${spN(x-.35*rx)} ${spN(y-.35*ry)} Z" fill="#ffffdc" opacity="${spN(.6+.4*blink)}"/>`;
 }
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${-.7*w} ${-1.6*h} ${2.4*w} ${3.8*h}" data-combat="eject" data-camera="${o.view}-axial" data-native-ko-frame="${index}" data-native-engine-frame="${F.engineFrame}" data-native-rope-frame="${F.ropeFrame}" data-native-visible="${F.valid}" data-angle-frame="${frame}" data-native-mirror="${F.mirror}" data-pose-source="Actual C++ KO flight at 60 Hz" data-angle-art="${origin}">${behind}${actor}${overlay}${over}</svg>`;
}

const JOANA_NATIVE_ARMOR={"front":"<g>\n                \n                <g transform=\"translate(34, 46)\">\n                    <polygon points=\"-12,8 -6,-12 8,-10 14,8 8,14 -8,12\" fill=\"#4a3728\" stroke=\"#0f1c10\" stroke-width=\"2\"/>\n                    <polygon points=\"-8,6 -4,-8 6,-6 10,6 6,10 -6,8\" fill=\"#8c7355\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"1\" cy=\"-4\" rx=\"5\" ry=\"3.5\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"1\" cy=\"-4\" rx=\"3\" ry=\"2\" fill=\"#fde047\"/>\n                    <polygon points=\"-2,-4 1,-1 4,-4\" fill=\"#ffffff\" stroke=\"#0f1c10\" stroke-width=\"0.8\"/>\n                    \n                    <circle cx=\"-7\" cy=\"4\" r=\"3.5\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <circle cx=\"-7\" cy=\"4\" r=\"1.8\" fill=\"#f59e0b\"/>\n                </g>\n\n                \n                <g transform=\"translate(86, 46)\">\n                    <polygon points=\"-14,8 -8,-10 6,-12 12,8 8,12 -8,14\" fill=\"#4a3728\" stroke=\"#0f1c10\" stroke-width=\"2\"/>\n                    <polygon points=\"-10,6 -6,-6 4,-8 8,6 6,8 -6,10\" fill=\"#8c7355\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"-1\" cy=\"-4\" rx=\"5\" ry=\"3.5\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"-1\" cy=\"-4\" rx=\"3\" ry=\"2\" fill=\"#fde047\"/>\n                    <polygon points=\"-4,-4 -1,-1 2,-4\" fill=\"#ffffff\" stroke=\"#0f1c10\" stroke-width=\"0.8\"/>\n                    \n                    <circle cx=\"7\" cy=\"4\" r=\"3.5\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <circle cx=\"7\" cy=\"4\" r=\"1.8\" fill=\"#f59e0b\"/>\n                </g>\n\n                \n                <g transform=\"translate(60, 62)\">\n                    \n                    <polygon points=\"-18,-14 0,-18 18,-14 20,8 12,16 -12,16 -20,8\" fill=\"#4a3728\" stroke=\"#0f1c10\" stroke-width=\"2\"/>\n                    <polygon points=\"-14,-11 0,-14 14,-11 16,6 10,12 -10,12 -16,6\" fill=\"#8c7355\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <polygon points=\"-10,-8 0,-10 10,-8 12,4 8,9 -8,9 -12,4\" fill=\"#ba9e7b\"/>\n\n                    \n                    <path d=\"M 0 -14 L 0 10 M -12 -8 L 8 10 M 12 -8 L -8 10\" stroke=\"#fde047\" stroke-width=\"1.5\" opacity=\"0.85\"/>\n\n                    \n                    <ellipse cx=\"0\" cy=\"0\" rx=\"9\" ry=\"7.5\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.8\"/>\n                    <ellipse cx=\"0\" cy=\"0\" rx=\"6.5\" ry=\"5\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"0\" cy=\"0\" rx=\"4\" ry=\"3\" fill=\"#fde047\"/>\n                    \n                    <polygon points=\"-3,-4 0,-1 3,-4\" fill=\"#ffffff\" stroke=\"#0f1c10\" stroke-width=\"0.8\"/>\n                    <polygon points=\"-3,4 0,1 3,4\" fill=\"#ffffff\" stroke=\"#0f1c10\" stroke-width=\"0.8\"/>\n\n                    \n                    <circle cx=\"-10\" cy=\"8\" r=\"4\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <circle cx=\"-10\" cy=\"8\" r=\"2\" fill=\"#f59e0b\"/>\n                    <circle cx=\"10\" cy=\"8\" r=\"4\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <circle cx=\"10\" cy=\"8\" r=\"2\" fill=\"#f59e0b\"/>\n                    \n                    \n                    <path d=\"M -16 6 C -20 12 -18 18 -14 20\" fill=\"none\" stroke=\"#2d6a4f\" stroke-width=\"1.8\" stroke-linecap=\"round\"/>\n                    <path d=\"M 16 6 C 20 12 18 18 14 20\" fill=\"none\" stroke=\"#2d6a4f\" stroke-width=\"1.8\" stroke-linecap=\"round\"/>\n                </g>\n\n                \n                <g transform=\"translate(60, 42)\">\n                    <polygon points=\"-12,-2 -8,-8 -4,-2\" fill=\"#ba9e7b\" stroke=\"#0f1c10\" stroke-width=\"1\"/>\n                    <polygon points=\"-4,-2 0,-9 4,-2\" fill=\"#ba9e7b\" stroke=\"#0f1c10\" stroke-width=\"1\"/>\n                    <polygon points=\"4,-2 8,-8 12,-2\" fill=\"#ba9e7b\" stroke=\"#0f1c10\" stroke-width=\"1\"/>\n                </g>\n            </g>","back":"<g>\n                \n                <g transform=\"translate(34, 46)\">\n                    <polygon points=\"-14,8 -8,-12 6,-10 12,8 6,14 -8,12\" fill=\"#4a3728\" stroke=\"#0f1c10\" stroke-width=\"2\"/>\n                    <polygon points=\"-10,6 -6,-8 4,-6 8,6 4,10 -6,8\" fill=\"#8c7355\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"-1\" cy=\"-4\" rx=\"5\" ry=\"3.5\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"-1\" cy=\"-4\" rx=\"3\" ry=\"2\" fill=\"#fde047\"/>\n                </g>\n\n                \n                <g transform=\"translate(86, 46)\">\n                    <polygon points=\"-12,8 -6,-10 8,-12 14,8 8,12 -8,14\" fill=\"#4a3728\" stroke=\"#0f1c10\" stroke-width=\"2\"/>\n                    <polygon points=\"-8,6 -4,-6 6,-8 10,6 6,8 -6,10\" fill=\"#8c7355\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"1\" cy=\"-4\" rx=\"5\" ry=\"3.5\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1.2\"/>\n                    <ellipse cx=\"1\" cy=\"-4\" rx=\"3\" ry=\"2\" fill=\"#fde047\"/>\n                </g>\n\n                \n                <g transform=\"translate(60, 60)\">\n                    <polygon points=\"-24,-16 0,-24 24,-16 28,12 16,22 -16,22 -28,12\" fill=\"#4a3728\" stroke=\"#0f1c10\" stroke-width=\"2.5\"/>\n                    <polygon points=\"-20,-12 0,-18 20,-12 24,8 14,18 -14,18 -24,8\" fill=\"#8c7355\" stroke=\"#0f1c10\" stroke-width=\"1.5\"/>\n                    <polygon points=\"-16,-8 0,-14 16,-8 18,4 10,14 -10,14 -18,4\" fill=\"#ba9e7b\"/>\n\n                    \n                    <path d=\"M 0 -18 L 0 16 M -18 -10 L 14 16 M 18 -10 L -14 16\" stroke=\"#fde047\" stroke-width=\"1.5\" opacity=\"0.85\"/>\n\n                    \n                    \n                    <g transform=\"translate(0, -6)\">\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"8\" ry=\"6.5\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.5\"/>\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"5.5\" ry=\"4.2\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1\"/>\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"3.5\" ry=\"2.5\" fill=\"#fde047\"/>\n                        <polygon points=\"-2.5,-3 0,-0.5 2.5,-3\" fill=\"#ffffff\" stroke=\"#0f1c10\" stroke-width=\"0.8\"/>\n                    </g>\n                    \n                    <g transform=\"translate(-10, 8)\">\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"7\" ry=\"5.5\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.5\"/>\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"4.8\" ry=\"3.5\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1\"/>\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"3\" ry=\"2\" fill=\"#fde047\"/>\n                    </g>\n                    \n                    <g transform=\"translate(10, 8)\">\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"7\" ry=\"5.5\" fill=\"#5c4d3c\" stroke=\"#0f1c10\" stroke-width=\"1.5\"/>\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"4.8\" ry=\"3.5\" fill=\"#d97706\" stroke=\"#0f1c10\" stroke-width=\"1\"/>\n                        <ellipse cx=\"0\" cy=\"0\" rx=\"3\" ry=\"2\" fill=\"#fde047\"/>\n                    </g>\n\n                    \n                    <path d=\"M -22 10 C -26 16 -24 24 -20 28\" fill=\"none\" stroke=\"#2d6a4f\" stroke-width=\"2\" stroke-linecap=\"round\"/>\n                    <path d=\"M 22 10 C 26 16 24 24 20 28\" fill=\"none\" stroke=\"#2d6a4f\" stroke-width=\"2\" stroke-linecap=\"round\"/>\n                </g>\n            </g>"};
// Intentional visual revisions requested after the native-motion audit.
// Native casting, recovery and hand/body tracks remain the timing authority.
const revisionWindow=(t,a,b,r=.12)=>spEase((t-a)/r)*(1-spEase((t-b)/r));
const revisionMatrix=(s,m)=>`<g transform="matrix(${m.map(spN).join(' ')})">${s}</g>`;
const revisionTimingCache=new WeakMap();
function revisionTiming(s,view){
 let cached=revisionTimingCache.get(s);if(!cached){cached={};revisionTimingCache.set(s,cached);}
 if(cached[view])return cached[view];
 const frames=s.native?.views[view==='back'?'back':'front']?.frames||[],initial=frames[0]?.state;
 const first=predicate=>{const i=frames.findIndex(predicate);return i<0?null:i/60;};
 // Hit stop freezes the native timer, so nominal skill.duration is not the
 // elapsed release/recovery time. Use the captured state transitions.
 let apex=0;
 if(s.key==='joana_claw_crush')for(let i=1;i<frames.length&&frames[i].state===initial;i++)if(frames[i].gloves[1][2]>frames[apex].gloves[1][2])apex=i;
 return cached[view]={event:first(f=>f.state===initial&&1-f.timer/f.max>=s.at)??s.duration*s.at,castEnd:first(f=>f.state!==initial)??s.duration,clamp:first(f=>String(f.claw).toLowerCase()==='clamp')??s.duration*s.at,apex:apex/60};
}
function joanaCrushMotion(s,view,t){
 const timing=revisionTiming(s,view),snap=timing.apex;
 // Finish the quick squeeze at maximum world-space reach, before retraction.
 const close=spEase((t-(snap-.05))/.05),release=1-spEase((t-timing.castEnd)/.24);
 const opened=t<snap-.05?-.3-.7*spEase(t/Math.max(.05,snap-.10)):-1;
 const jaw=(opened+(2-opened)*close)*release,elapsed=t-snap;
 const amplitude=elapsed>=0&&elapsed<.20?3.4*Math.pow(1-elapsed/.20,2):0;
 return {jaw,snap,shake:amplitude?[spN(Math.cos(elapsed*150)*amplitude),spN(Math.sin(elapsed*123+.7)*amplitude*.65)]:[0,0]};
}
function joanaStableCrushCapture(s,view,capture,t){
 if(!capture)return capture;
 const sequence=s.native.views[view==='back'?'back':'front'];
 // The captured camera already shook at the damage event, BEFORE the native
 // Clamp sprite. Remove that common screen translation from body AND hands;
 // the gallery applies one short viewport shake at the new actual jaw contact.
 const first=sequence.frames[0],clean=sequence.frames.findLast(f=>f.state===first.state&&1-f.timer/f.max<.40)||first,f=capture.frame;
 // In this axial thrust, x/y/depth are all driven by the same native sine.
 // Calibrate perspective from two clean frames: inverse apparent width is
 // linear in world depth, screen center is affine in apparent width. This
 // removes captured yaw/depth shake too, not just a common 2D translation.
 const project=(a,b,z0,z1,z,original,height=false)=>{
  if(Math.abs(z1-z0)<1e-6)return original;
  const inverse=1/a[2]+(1/b[2]-1/a[2])*(z-z0)/(z1-z0),width=1/inverse,u=(width-a[2])/(b[2]-a[2]);
  const out=[...original];out[0]=a[0]+(b[0]-a[0])*u;out[1]=a[1]+(b[1]-a[1])*u;out[2]=width;if(height)out[3]=width*a[3]/a[2];return out;
 };
 const recovery=spEase((t-revisionTiming(s,view).castEnd)/.16),restore=(stable,original)=>stable.map((v,i)=>typeof v==='number'?v+(original[i]-v)*recovery:v);
 return {...capture,frame:{...f,screen:restore(project(first.screen,clean.screen,first.body[2],clean.body[2],f.body[2],f.screen,true),f.screen),hands:f.hands.map((h,i)=>restore(project(first.hands[i],clean.hands[i],first.gloves[i][2]+first.body[2],clean.gloves[i][2]+clean.body[2],f.gloves[i][2]+f.body[2],h),h))}};
}
const revisionEmitterCache=new WeakMap();
function revisionEmitter(o,s,cfg,index,event){
 let cache=revisionEmitterCache.get(s);if(!cache){cache=new Map();revisionEmitterCache.set(s,cache);}
 const key=[o.view,index,cfg.w,cfg.h].join('/');if(cache.has(key))return cache.get(key);
 const seq=s.native?.views[o.view==='back'?'back':'front'],f=seq?.frames[Math.min(seq.frames.length-1,Math.floor((event+index*.075)*60))];
 const data=spNativeData(o,s,f?spClamp(1-f.timer/f.max):s.at,cfg,f?{frame:f,rest:seq.rest}:null),center=data.centers[index%2];cache.set(key,center);return center;
}
function joanaAttachedSpecial(state,s,t,native,total){
 if(!['joana_crustacean_shell','joana_ocean_bounty'].includes(s.key))return;
 const si=skillSkinIndex,back=state.view==='back',grow=spEase((t-.04)/.30),fade=1-spEase((t-(total-.30))/.25),ink=['#151c23','#17252b','#082332','#252d2c'][si];
 const path=(d,fill,stroke=ink,w=1.3)=>`<path d="${d}" fill="${fill}" stroke="${stroke}" stroke-width="${w}" stroke-linejoin="round" stroke-linecap="round"/>`;
 let art='';
 if(s.key==='joana_crustacean_shell'){
  const source=['#4a3728','#8c7355','#ba9e7b','#5c4d3c','#d97706','#fde047','#f59e0b','#2d6a4f','#ffffff','#0f1c10'];
  const palette=[source,['#42606d','#8cb7c4','#d7edf0','#507a89','#6ca5b5','#ecffff','#b6e4ec','#6e9a8d','#ffffff',ink],['#0b293d','#164f63','#287888','#123b50','#248f99','#a6ffee','#51dfd5','#43bdac','#d6fff5',ink],['#543d30','#966b44','#c8985d','#453b32','#9b8b61','#ffe6a2','#cbbd8e','#897751','#e4d6b5',ink]][si];
  const colors=Object.fromEntries(source.map((c,i)=>[c,palette[i]]));colors['#0f1c10']=ink;
  art=JOANA_NATIVE_ARMOR[back?'back':'front'].replace(/#[0-9a-f]{6}/gi,c=>colors[c.toLowerCase()]||c);
  let detail='';
  if(si===1)detail=path('M27 53 L35 43 L37 52 M84 46 L89 54 L96 51 M48 60 L55 55 L57 63 M66 70 L73 65 L75 73','none','#f3ffff',1.5);
  if(si===2)for(const [x,y]of (back?[[60,54],[50,68],[70,68]]:[[60,62],[35,42],[85,42]]))for(let j=-1;j<=1;j++)detail+=path(`M${x+j*3} ${y-2} Q${x+j*6} ${y-10} ${spN(x+j*7+Math.sin(t*3+j)*1.5)} ${y-15}`,'none','#68dace',1.8);
  if(si===3){for(const [x,y]of [[34,46],[86,46],[45,55],[75,55],[45,71],[75,71]])detail+=`<circle data-armor-rivet="true" cx="${x}" cy="${y}" r="1.8" fill="#e4d6b5" stroke="${ink}" stroke-width=".8"/>`;detail+=path('M49 58 H71 M49 64 H71 M49 70 H71','none','#d9b278',1.3);}
  // Refit vertically to the approved torso. Keep BOTH large shoulder plates,
  // full breast/back plate and native barnacle sockets; never clip them to a bib.
  state.specialBodyOverlay=revisionMatrix(`<g data-attached-armor="${['barnacle','arctic-salt-ice','abyssal-anemones','riveted-boiler'][si]}" data-armor-source="native-barnacle-vector" data-armor-view="${back?'back':'front'}" data-armor-growth="${spN(grow)}" opacity="${spN(fade)}"><g transform="translate(60 66) scale(${spN(.90+.10*grow)}) translate(-60 -66)" opacity="${spN(grow)}"><g data-native-armor-fit="torso" transform="translate(0 18.5) scale(1 .78)">${art}${detail}</g></g></g>`,native.body);
 }else{
  // Every stalk starts on the upper back. The front camera sees only the
  // silhouette around the coat; rear camera sees roots resting on the fabric.
  for(let i=0;i<5;i++){
   const x=[39,49,60,71,81][i],y=[72,66,69,65,72][i],scale=grow*[.8,1,.88,.94,.76][i],sway=Math.sin(t*3.6+i)*2;
   let flower='';
   if(si===3){flower=path(`M-3 3 V-10 Q-3 -15 ${4+sway} -15 H${8+sway}`,'none','#a87b4c',4)+path(`M-3 3 V-10 Q-3 -15 ${4+sway} -15 H${8+sway}`,'none','#dfbc79',1.5)+`<circle cx="${spN(8+sway)}" cy="-15" r="3.2" fill="#496d67" stroke="${ink}" stroke-width="1.2"/><path d="M${spN(6+sway)} -15 h4 m-2 -2 v4" stroke="#d6c198" stroke-width="1"/>`;}
   else{
    const color=si===1?'#5c9b82':si===2?'#2ba6b6':'#b44386',light=si===1?'#d7eee0':si===2?'#b6fff0':'#f3a7c8';
    flower=path('M-4 3 Q-7 -3 -3 -8 Q0 -11 4 -7 Q7 -2 4 3 Z',color)+path('M-2 1 Q0 3 2 1','none',light,1.2);
    for(let j=0;j<5;j++){const a=-Math.PI+(j+1)*Math.PI/6,dx=Math.cos(a)*9,dy=-10-Math.sin((j+1)*Math.PI/6)*10;flower+=path(`M0 -5 Q${spN(dx*.6+sway)} -10 ${spN(dx+sway)} ${spN(dy)}`,'none',color,2.5)+`<circle cx="${spN(dx+sway)}" cy="${spN(dy)}" r="1.6" fill="${light}" stroke="${ink}" stroke-width=".7"/>`;}
   }
   art+=`<g data-back-root="${x} ${y}" transform="translate(${x} ${y}) scale(${spN(scale)})">${flower}</g>`;
  }
  state.specialBackAdornment=revisionMatrix(`<g data-attached-balm="${['sea-anemones','northern-kelp','abyssal-polyps','steam-valves'][si]}" data-growth-owner="upper-back" opacity="${spN(fade)}">${art}</g>`,native.body);
 }
}
function revisedSpecialEffects(o,s,t,cfg,capture,native,total){
 const id=skillCharacter,si=skillSkinIndex,key=s.key,back=o.view==='back';
 const joana=id==='joana'&&['joana_crustacean_shell','joana_ocean_bounty','joana_claw_crush'].includes(key),cream=id==='cremoso'&&/syrup_blast|cone_punch|puddle_trap|melt_mode/.test(key),primate=id==='marcov'&&[1,3].includes(si),robot=id==='batalhator'&&(/orbital_apocalypse/.test(key)||si>0&&/overclock|emp|rocket_barrage/.test(key));
 if(!joana&&!cream&&!primate&&!robot)return null;
 const ink='#172027',pal=[['#975336','#e6ba78'],['#65863b','#c6db85'],['#a95976','#f1b0c1'],['#7b443b','#f4bf99']][si],timing=revisionTiming(s,o.view),event=timing.event,life=revisionWindow(t,.025,total-.28,.18),C=native.centers;
 let under='',over='';
 const path=(d,fill='none',stroke=ink,w=1.5)=>`<path d="${d}" fill="${fill}" stroke="${stroke}" stroke-width="${w}" stroke-linecap="round" stroke-linejoin="round"/>`;
 const circle=(x,y,r,c,stroke='none',w=1)=>`<circle cx="${spN(x)}" cy="${spN(y)}" r="${spN(Math.max(0,r))}" fill="${c}" stroke="${stroke}" stroke-width="${w}"/>`;
 const ell=(x,y,rx,ry,c,stroke='none',w=1)=>`<ellipse cx="${spN(x)}" cy="${spN(y)}" rx="${spN(rx)}" ry="${spN(ry)}" fill="${c}" stroke="${stroke}" stroke-width="${w}"/>`;
 const project=(x,y,z)=>{const k=260/(260+(back?1:-1)*z);return [cfg.w/2+(x-cfg.w/2)*k,cfg.h*.55+(y-cfg.h*.55)*k,k];};
 const at=(x,y,z,art,scale=1,angle=0)=>{const q=project(x,y,z);return `<g transform="translate(${spN(q[0])} ${spN(q[1])}) scale(${spN(q[2]*scale)}) rotate(${spN(angle)})">${art}</g>`;};
 const put=(art,z=0,a=1,forceBack=false)=>{if(a<.001)return;const g=`<g opacity="${spN(spClamp(a))}">${art}</g>`;if(forceBack||z<0&&!back||z>0&&back)under+=g;else over+=g;};
 const burst=(x,y,z,start,color,count=9)=>{const u=(t-start)/.35;if(u<=0||u>=1)return;for(let i=0;i<count;i++){const a=i*2.399,r=7+u*(12+i%3*4);put(at(x+Math.cos(a)*r,y+Math.sin(a)*r,z,path('M-1 -3 L2 0 L0 3 L-2 0 Z',color,ink,.6),1,i*31),z,Math.sin(u*Math.PI));}};
 const cloud=(x,y,z,start,color,count=9,duration=.7)=>{for(let i=0;i<count;i++){const u=(t-start-i*.035)/duration;if(u<=0||u>=1)continue;const dx=Math.cos(i*2.399)*(3+u*18),dy=Math.sin(i*2.399)*(2+u*8)-u*12;put(at(x+dx,y+dy,z,circle(0,0,2+u*(4+i%3),color)),z,Math.sin(u*Math.PI)*.48);}};
 if(joana&&key==='joana_claw_crush'){
  // A short contact accent leaves the actual meeting of the jaws readable.
  // Native hand trajectory is retained; jaw contact is retimed to full reach.
  const color=['#f1c286','#d9f6ee','#9cf3e4','#f4ce7d'][si];
  burst(C[1][0],C[1][1],10,timing.apex,color,7);
 }else if(cream){
  if(key==='cremoso_syrup_blast'){
   for(let i=0;i<3;i++){
    const u=(t-event-i*.075)/.68;if(u<=0||u>=1)continue;
    const hand=revisionEmitter(o,s,cfg,i,event),z=170*u,tail=Math.max(0,u-.16),spread=(i-1)*16;
    const point=v=>project(hand[0]*(1-v)+(60+spread)*v,hand[1]*(1-v)+(back?-90:67)*v-12*Math.sin(v*Math.PI),170*v);
    const A=point(tail),B=point(u),a=Math.sin(u*Math.PI),size=.35+u*.85;
    const trail=path(`M${spN(A[0])} ${spN(A[1])} L${spN(B[0])} ${spN(B[1])}`,'none',ink,5*B[2])+path(`M${spN(A[0])} ${spN(A[1])} L${spN(B[0])} ${spN(B[1])}`,'none',pal[0],3*B[2]);
    const splash=path('M-24 0 Q-17 -10 -12 -4 Q-7 -13 -1 -5 Q8 -11 12 -3 Q18 -8 24 0 Q16 5 12 13 Q3 8 -3 13 Q-13 7 -24 0 Z',pal[0],ink,1.6)+path('M-16 0 Q-8 6 0 2 Q8 -1 16 2 M-7 7 Q0 9 5 6','none',pal[1],2);
    const sheet=`<g transform="translate(${spN(B[0])} ${spN(B[1])}) scale(${spN(B[2]*size)}) rotate(${i*19-19})">${splash}</g>`;
    put(`<g data-airborne-syrup="${i}" data-moving-tail="${spN(tail)}" data-no-ground-puddle="true">${trail}${sheet}</g>`,z,a);
    for(let j=0;j<2;j++)put(at(60+spread*u+(j?1:-1)*(12+u*14),(back?-6:74)+(back?-96:8)*u,z,ell(0,0,2.8,4.5,pal[0],ink,1),1,i*37+j*55),z,a);
   }
  }else if(key==='cremoso_cone_punch'){
   const rise=spEase((t-(event-.12))/.20)*(1-spEase((t-(event+.25))/.25)),m=native.body;
   const colors=[['#d5a45e','#86532f','#f5d79a'],['#7f9a46','#3f652f','#d3e2a2'],['#d7b17a','#805536','#ffe6b9'],['#f3d4a7','#ba6b91','#ffeada']][si],theme=['waffle','matcha-leaf','layered-wafer','sugar-wafer'][si];
   const face=(origin,r)=>{
    let lattice='';
    for(let i=-4;i<=4;i++)lattice+=path(`M${-r} ${i*6-r} L${r} ${i*6+r} M${-r} ${i*6+r} L${r} ${i*6-r}`,'none',colors[1],.8);
    const clip='cream-cone-face-'+origin;
    return `<g data-cone-origin="${origin}" data-cone-theme="${theme}" data-cone-rise="${spN(rise)}" data-cone-projection="circular-axial"><defs><clipPath id="${clip}">${circle(0,0,r-2,'#fff')}</clipPath></defs>${circle(0,0,r,pal[0],ink,2)}${circle(0,0,r-2,colors[0],colors[1],1)}<g clip-path="url(#${clip})">${path(`M-3 -2 L${r} ${-r} V${r} Z`,colors[1],'none')}${si===3?path(`M${-r} 0 L0 ${r} M0 ${-r} L${r} 0`,'none','#ed8eb3',3):''}${lattice}</g>${path(`M${-r*.64} ${-r*.35} Q${-r*.6} ${-r*.7} ${-r*.28} ${-r*.78}`,'none',colors[2],1.6)}${path('M-5 0 L-3 -3 L-1 0',colors[2],colors[1],.8)}</g>`;
   };
   const side=(origin,x,y,tipX,tipY)=>{
    const Y=y+(tipY-y)*rise,X=x+(tipX-x)*rise,half=origin==='chest'?22:9,d=`M${x-half} ${y} L${spN(X)} ${spN(Y)} L${x+half} ${y} Z`;
    const clip='cream-cone-'+origin;
    let mesh='';for(let i=1;i<7;i++){const f=i/7;mesh+=path(`M${spN(x-9+9*f)} ${spN(y+(Y-y)*f)} L${spN(x+9-9*f)} ${spN(y+(Y-y)*f)} M${x-8+i*2} ${y} L${spN(X)} ${spN(Y)}`,'none',colors[1],.9);}
    return `<g data-cone-origin="${origin}" data-cone-theme="${theme}" data-cone-projection="triangle-tip" data-cone-rise="${spN(rise)}"><defs><clipPath id="${clip}">${path(d)}</clipPath></defs>${path(d,colors[0],ink,2)}<g clip-path="url(#${clip})">${mesh}${si===3?path(`M${x-14} ${y} L${spN(X+5)} ${spN(Y)}`,'none','#ed8eb3',3):''}</g>${path(`M${spN(X-1)} ${spN(Y+7)} L${spN(X)} ${spN(Y+3)}`,'none',colors[2],1.2)}</g>`;
   };
   if(rise>.001){
    if(back){
     under+=revisionMatrix(`<g opacity="${spN(rise)}">${side('chest',60,76,60,-28)}</g>`,m);
     over+=revisionMatrix(`<g opacity="${spN(rise)}">${side('back-left',36,76,18,17)}${side('back-right',84,76,102,17)}</g>`,m);
    }else{
     under+=revisionMatrix(`<g opacity="${spN(rise)}">${side('back-left',36,76,12,29)}${side('back-right',84,76,108,29)}</g>`,m);
     over+=revisionMatrix(`<g transform="translate(60 76) scale(${spN(rise)})" opacity="${spN(rise)}">${face('chest',20)}</g>`,m);
    }
   }
  }
  // Puddle and melt have one authored body silhouette, not an extra floor FX.
 }else if(primate){
  const stone=si===1,color=stone?'#bd9d70':'#e5ddca',accent=stone?'#f0cc7b':'#c95d41',cx=60,ground=130,m=native.body,bodyY=m[5];
  const prop=o.specialParts?.orbital||'',orbital=prop?`<g data-native-themed-prop="orbital-${si}">${prop}</g>`:'';
  if(key==='marcov_rocket_thrust'){cloud(cx,ground,-12,.05,color,14,.75);burst(C[1][0],C[1][1],75,event,accent);}
  else if(key==='marcov_orbital_laser'){
   if(stone){const active=revisionWindow(t,event,event+.48,.08),bolt=path('M60 -35 L48 -8 L64 -12 L49 21 L65 17 L57 57 L67 53 L60 108','none','#6f4c31',6)+path('M60 -35 L48 -8 L64 -12 L49 21 L65 17 L57 57 L67 53 L60 108','none','#f7df8c',3);put(`<g data-primordial-strike="lightning">${bolt}</g>`,85,active,back);burst(60,108,85,event+.12,accent,14);}
   else{
    const pass=1.55,u=spClamp((t-.02)/pass),planeX=-40+200*u,planeY=-72+Math.sin(u*Math.PI)*10,alpha=revisionWindow(t,.02,1.42,.12);
    const trail=path(`M${spN(planeX-13)} ${spN(planeY+2)} Q${spN(planeX-40)} ${spN(planeY+11)} ${spN(Math.max(-58,planeX-95))} ${spN(planeY+4)}`,'none','#dce1cf',3)+path(`M${spN(planeX-12)} ${spN(planeY-4)} H${spN(Math.max(-58,planeX-60))}`,'none','#a9c2bf',1.5);
    put(`<g data-aviator-air-raid="biplane" data-aircraft-width="78" transform="translate(${spN(planeX)} ${spN(planeY)}) scale(2.3)">${orbital}</g>`,0,alpha);
    put(trail,0,alpha*.7);
    for(let i=0;i<6;i++)for(let j=0;j<2;j++){
     const start=event+i*.105+j*.025,v=(t-start)/.58;if(v<=0||v>=1)continue;
     const sx=-40+200*spClamp((start-.02)/pass),sy=-62,tx=35+i*9+(j?8:-8),ty=back?-6:179;
     const x=sx+(tx-sx)*v,y=sy+(ty-sy)*v,old=Math.max(0,v-.15),tailX=sx+(tx-sx)*old,tailY=sy+(ty-sy)*old,scale=back?1-v*.25:.7+v*.55;
     const shell=ell(0,0,3.3,6.5,'#e7c379',ink,1)+path('M-1 -3 V1','none','#fff1bb',1.2);
     put(`<g data-air-raid-shell="${i}-${j}" data-flight-progress="${spN(v)}">${path(`M${spN(tailX)} ${spN(tailY)} L${spN(x)} ${spN(y)}`,'none','#eee1ae',2.2)}<g transform="translate(${spN(x)} ${spN(y)}) rotate(${spN(-Math.atan2(tx-sx,ty-sy)*180/Math.PI)}) scale(${spN(scale)})">${shell}</g></g>`,0,Math.min(1,v*12)*(1-spEase((v-.82)/.18)),back);
     burst(tx,ty,0,start+.48,'#efb464',7);
    }
    put(`<g data-aviator-signal-flare="true" transform="translate(60 -35)">${path('M0 -10 L3 -3 L9 0 L3 3 L0 10 L-3 3 L-9 0 L-3 -3 Z','#db7650')}${circle(0,0,3,'#ffeab1')}</g>`,0,revisionWindow(t,.03,event+.15,.06));
   }
  }else if(key==='marcov_plasma_barrier'){
   for(let i=0;i<(stone?6:0);i++){const angle=t*2.3+i*Math.PI/3,z=Math.sin(angle)*35,x=60+Math.cos(angle)*43,y=69+bodyY+Math.sin(angle*2)*8;
    const rock=path('M-5 -2 L-2 -6 L4 -5 L7 1 L2 6 L-5 4 Z','#897253',ink,1.2)+path('M-2 -3 L1 1 L-1 3','none','#d4b37c',1);
    put(at(x,y,z,stone?rock:orbital,stone?1:.8),z,life);
   }
   if(!stone){
    for(let i=0;i<3;i++){
     const rx=62+i*4,ry=76+i*4,y=67+bodyY,offset=-t*58+i*15,stroke=i===1?'#f1e6ca':'#adc5c3';
     for(const near of [false,true]){
      const arc=`M${60-rx} ${spN(y)} A${rx} ${ry} 0 0 ${near?0:1} ${60+rx} ${spN(y)}`;
      const art=`<g data-aviator-wind-shield="${near?'near':'far'}" data-shield-radius="${rx}" stroke-dasharray="${i===1?'53 12':'27 17'}" stroke-dashoffset="${spN(offset)}">${path(arc,'none',stroke,i===1?2.8:1.5)}</g>`;
      put(art,0,life*(i===1?.85:.62),!near);
     }
    }
    const angle=t*2.8-Math.PI/2,x=60+Math.cos(angle)*70,y=67+bodyY+Math.sin(angle)*79;
    put(`<g data-aviator-shield-plane="true" transform="translate(${spN(x)} ${spN(y)}) rotate(${spN(Math.atan2(79*Math.cos(angle),-70*Math.sin(angle))*180/Math.PI)}) scale(1.3)">${orbital}</g>`,0,life,Math.sin(angle)<0);
   }
  }else if(key==='marcov_kinetic_overdrive'){
   for(let i=0;i<5;i++)put(at(C[1][0],C[1][1],20,path(`M${-12-i*2} ${-8+i*4} h8`,'none',stone?'#b18d5c':'#e5dcc6',1.7)),20,revisionWindow(t,.05,event+.2,.08));burst(C[1][0],C[1][1],90,event,accent,13);
  }else if(key==='marcov_exhaust_cloud')cloud(60,100+bodyY,10,.04,color,18,1.05);
  else{for(let i=0;i<12;i++){const u=(t-.08-i*.035)/.75;if(u<=0||u>=1)continue;const art=stone?path('M-3 0 Q0 -7 4 -5 Q5 0 0 3 Z','#92a46b',ink,.7)+path('M-1 1 L3 -3','none','#d4ce92',.6):path('M-4 1 Q0 -4 5 0','none','#d9e6e2',1.3);put(at(60+Math.sin(i*2.4)*(24+u*8),111-u*53,Math.sin(i)*20,art,1,i*13),Math.sin(i)*20,Math.sin(u*Math.PI));}}
 }else if(robot){
  const colors=[['#ef6559','#c54a49','#ffe3a0'],['#28c6df','#eb62c1','#f7d060'],['#b88a50','#e3c38b','#c1d0c8'],['#66cde9','#ed655c','#f4d269']][si],cx=cfg.w/2,cy=cfg.h*.48,theme=['industrial','arcade-pixels','steam-pressure','sentai-plasma'][si];
  const glyph=(r,c)=>si===1?path(`M${-r} ${-r*.45} H${-r*.45} V${-r} H${r*.45} V${-r*.45} H${r} V${r*.45} H${r*.45} V${r} H${-r*.45} V${r*.45} H${-r} Z`,'none',c,2):si===2?path(Array.from({length:24},(_,j)=>{const a=j*Math.PI/12,R=j%3?r:r*.84;return `${j?'L':'M'}${spN(Math.cos(a)*R)} ${spN(Math.sin(a)*R)}`;}).join(' ')+' Z','none',c,2):path(`M0 ${-r} L${r*.24} ${-r*.25} L${r} 0 L${r*.24} ${r*.25} L0 ${r} L${-r*.24} ${r*.25} L${-r} 0 L${-r*.24} ${-r*.25} Z`,'none',c,2);
  if(key==='batalhator_overclock_overdrive'){
   let glow='';for(let i=0;i<3;i++)glow+=at(cx,cy,0,glyph(29+i*6,colors[i]),1,t*(si===2?60:20)*(i%2?-1:1));put(revisionMatrix(`<g data-overclock-core="${theme}">${glow}</g>`,native.body),0,life*.85);
   if(si===2)cloud(cx,cy-12,-8,event,'#c7d4cc',12,1);else burst(cx,cy,20,event,colors[0],15);
  }else if(key==='batalhator_emp_shockwave'){
   for(let i=0;i<3;i++){const u=(t-event-i*.08)/.60;if(u<=0||u>=1)continue;put(at(cx,cy,70*u,glyph(13+u*38,colors[i]),1,si===2?t*75:0),70*u,Math.sin(u*Math.PI));}
   burst(cx,cy,40,event,colors[2],12);
  }else if(key==='batalhator_rocket_barrage'){
   for(let i=0;i<6;i++){
    const u=(t-(event-.14)-i*.09)/.70;if(u<=0||u>=1)continue;const side=i%2?1:-1,color=colors[i%3],m=native.body;let projectile;
    if(si===1)projectile=path('M-6 -9 H6 V7 H-6 Z','#292538',ink,1.4)+path('M-4 -6 H4 V-1 H-4 Z',color,ink,.7)+path('M-4 5 H4','none','#e1c78c',1.4);
    else if(si===2)projectile=path('M-4 8 V-4 Q0 -10 4 -4 V8 Z','#a97b47',ink,1.5)+path('M-4 3 H4 M-2 -4 H2','none','#ebd4a5',1.2)+ell(0,8,4,1.8,'#5e6c68',ink,.8);
    else projectile=path('M0 -10 L3 -3 L10 0 L3 3 L0 10 L-3 3 L-10 0 L-3 -3 Z',color,ink,1.3)+circle(0,0,2.5,'#fbf0c8',ink,.6);
    // Rise clear of the launcher, then travel along the fight axis. The rear
    // camera sees the salvo receding above the head instead of inside the chest.
    const point=v=>{
     const start=[m[0]*(cx+side*48)+m[4],m[3]*35+m[5]],control=[cx+side*67,-105],end=[cx+side*(27+i%3*9),back?-104:188];
     const x=(1-v)**2*start[0]+2*(1-v)*v*control[0]+v*v*end[0],y=(1-v)**2*start[1]+2*(1-v)*v*control[1]+v*v*end[1];
     return project(x,y,115*v);
    };
    const B=point(u),A=point(Math.max(0,u-.13)),angle=-Math.atan2(B[0]-A[0],B[1]-A[1])*180/Math.PI+180,trailColor=si===2?'#cbd9cf':color;
    const trail=path(`M${spN(A[0])} ${spN(A[1])} L${spN(B[0])} ${spN(B[1])}`,'none',ink,4*B[2])+path(`M${spN(A[0])} ${spN(A[1])} L${spN(B[0])} ${spN(B[1])}`,'none',trailColor,2.5*B[2]);
    const art=`<g data-themed-projectile="${theme}" data-flight-progress="${spN(u)}" data-flight-center="${spN(B[0])} ${spN(B[1])}">${trail}<g transform="translate(${spN(B[0])} ${spN(B[1])}) scale(${spN(B[2])}) rotate(${spN(si===3?t*700*side:angle)})">${projectile}</g></g>`;
    put(art,115*u,Math.min(1,u*15)*(1-spEase((u-.80)/.20)),back);
   }
  }else{
   const ground=back?-17:205,top=-111,targetScale=back?.72:1,charge=revisionWindow(t,.035,event+.10,.10),alpha=revisionWindow(t,event-.025,timing.castEnd+.12,.11),width=(18+Math.sin(t*23)*2.3)*alpha,targetX=cx+Math.sin(t*9)*3*alpha;
   let target='';for(let i=0;i<2;i++)target+=ell(targetX,ground,(25+i*12)*targetScale,(7+i*3)*targetScale,'none',colors[i],i?1.5:2.5);
   target+=path(`M${targetX-47*targetScale} ${ground} h15 M${targetX+32*targetScale} ${ground} h15 M${targetX} ${ground-15*targetScale} v8 M${targetX} ${ground+7*targetScale} v8`,'none',colors[2],1.5);
   put(`<g data-apocalypse-target="${theme}">${target}</g>`,0,Math.max(charge,alpha)*.8,back);
   if(alpha>.001){
    const w=width*targetScale,quad=(f,c)=>path(`M${spN(cx-w*f*.65)} ${top} L${spN(cx+w*f*.65)} ${top} L${spN(targetX+w*f)} ${ground} L${spN(targetX-w*f)} ${ground} Z`,c,'none');
    let beam=`<g opacity=".22">${quad(1.7,colors[0])}</g>${quad(1.1,si===2?'#748680':colors[1])}${quad(.78,si===2?'#d5d7bd':colors[0])}${quad(.24,'#fff5d3')}`;
    if(si===3){beam='';for(let j=0;j<3;j++){const dx=(j-1)*w*.6;beam+=path(`M${spN(cx+dx*.65)} ${top} L${spN(targetX+dx)} ${ground}`,'none',colors[j],w*.60)+path(`M${spN(cx+dx*.65)} ${top} L${spN(targetX+dx)} ${ground}`,'none','#fff5d3',w*.13);}}
    for(let j=0;j<11;j++){
     const v=(j/11+t*2.4)%1,y=top+(ground-top)*v,x=cx+(targetX-cx)*v,half=w*(.65+v*.35),length=si===1?0:11+v*12;
     beam+=si===1?path(`M${spN(x-half)} ${spN(y)} H${spN(x+half)}`,'none',j%2?colors[2]:'#e8f8ec',1.2):path(`M${spN(x+(j%2?-1:1)*half*.55)} ${spN(y-length)} L${spN(x+(j%2?-1:1)*half*.55)} ${spN(y)}`,'none',si===2?'#f6efce':'#fff1cd',1.4);
    }
    if(si===2)for(let j=0;j<10;j++){const v=(j/10+t*1.4)%1,y=top+(ground-top)*v;beam+=`<g opacity=".45">${circle(cx+Math.sin(j*2.4+v*5)*w*.8,y,3+v*7,'#bdcdc1')}</g>`;}
    put(`<g data-themed-apocalypse="${theme}" data-beam-width="${spN(width)}" data-beam-phase="${spN((t*2.4)%1)}">${beam}</g>`,0,alpha,back);
    for(let i=0;i<3;i++){const v=((t-event)*2+i/3)%1;if(v<0)continue;put(`<g data-apocalypse-impact-ring="${i}">${ell(targetX,ground,(19+v*42)*targetScale,(5+v*12)*targetScale,'none',colors[i],2*(1-v)+.6)}</g>`,0,alpha*(1-v),back);}
   }
   burst(targetX,ground,0,event+.12,colors[2],18);
  }
 }
 return {under:`<g data-revised-effects="${key}" data-skin-theme="${skillSkinId}" data-revised-layer="behind">${under}</g>`,over:`<g data-revised-effects="${key}" data-skin-theme="${skillSkinId}" data-revised-layer="front">${over}</g>`,authored:true};
}

// Shared retargeter: native C++ skill poses, with deterministic SVG study effects.
const SP_I=[1,0,0,1,0,0],spN=n=>Number(n.toFixed(5)),spClamp=n=>Math.max(0,Math.min(1,n)),spEase=n=>{n=spClamp(n);return n*n*(3-2*n);};
const spMul=(a,b)=>[a[0]*b[0]+a[2]*b[1],a[1]*b[0]+a[3]*b[1],a[0]*b[2]+a[2]*b[3],a[1]*b[2]+a[3]*b[3],a[0]*b[4]+a[2]*b[5]+a[4],a[1]*b[4]+a[3]*b[5]+a[5]];
const spInner=s=>s.replace(/^\s*<svg\b[^>]*>/,'').replace(/<\/svg>\s*$/,'');
const spPropsCache=new Map();
function spConfig(o){
 const id=skillCharacter,skin=skillSkinId,si=skillSkinIndex,back=o.view==='back';
 if(id==='lady')return {w:200,h:357,anchors:skin==='original'?[[14,208],[186,208]]:[[9,211],[191,211]]};
 if(id==='batalhator')return +o.phase===3?{w:skin==='original'?120:150,h:146,anchors:skin==='original'?[[24,92],[96,92]]:[[31,85],[119,85]]}:{w:150,h:170,anchors:skin==='original'?[[14,84],[136,84]]:[[5,71],[145,71]]};
 let anchors={jack:skin==='0'?[[22,66],[98,66]]:skin==='1'?[[25,70.5],[95,70.5]]:[[27,64.5],[93,64.5]],frog:[[22,66],[98,66]],joana:si===1?[[26,68],[o.state==='monster'?94:92,o.state==='monster'?66:68]]:[[si>1?25:24,64],[94,66]],marcov:[[23,76],[97,76]],valeria:[[[24,58],[96,58]],[[16,62],[104,62]],[[16,62],[104,62]],[[20,60],[100,60]]][si],vivaldi:si===0?[[30,52],[92,52]]:si===1?[[16,59],[104,58]]:[[16,58],[104,58]],cremoso:si===0?[[16,66],[104,66]]:[[6,77],[114,77]]}[id];
 if(back&&id==='vivaldi'&&si>0)anchors=anchors.map(([x,y])=>[120-x,y]);
 return {w:120,h:{marcov:140,valeria:140,vivaldi:165}[id]||120,anchors};
}
function spSample(s,p){const k=spClamp(p)*(s.samples.length-1),a=s.samples[Math.floor(k)],b=s.samples[Math.min(s.samples.length-1,Math.ceil(k))];return a.map((x,i)=>x+(b[i]-x)*(k%1));}
function spNativeData(o,s,p,cfg,capture=null){
 const data=o.combat,V=o.view==='back'?'B':'F',rest=data.trajectories[V+'_idle'][0],S=spSample(s,p),baseY=skillCharacter==='frog'?38:['joana','cremoso','valeria'].includes(skillCharacter)?68:70;
 if(capture){const f=capture.frame;S.splice(0,10,...f.gloves.flat(),...f.body);}
 const envelope=capture?1:spEase(p/.08)*(1-spEase((p-.86)/.14)),sx=cfg.w/120,sy=cfg.h/120,screen=['jack','frog'].includes(skillCharacter)?1:V==='F'?-1:1;
 const dx=S[6]*.5*sx*envelope,dy=-S[7]*.5*sy*envelope,skew=0;
 const depth=S[8]/600*envelope,bodyScale=V==='B'?1/(1+depth):1+depth;
 let body=[bodyScale,0,skew,bodyScale,dx+(1-bodyScale)*cfg.w*.5,dy+(1-bodyScale)*cfg.h*.55];
 if(capture){const f=capture.frame.screen,r=capture.rest.screen,A=f[2]/r[2],D=f[3]/r[3];body=[A,0,0,D,(f[0]-r[0])*cfg.w/r[2]+(1-A)*cfg.w*.5,(f[1]-r[1])*cfg.h/r[3]+(1-D)*cfg.h];}
 const tr=JSON.parse(JSON.stringify(rest));tr[4]=0;
 const centers=[],handDelta=[];
 for(let i=0;i<2;i++){
  const jack=skillCharacter==='jack',frog=skillCharacter==='frog',left=rest[5+i][0]<rest[0],direction=jack||frog?(rest[5][0]<rest[0]?1:-1):(cfg.anchors[0][0]<cfg.anchors[1][0]?1:-1);
  const g=S.slice(i*3,i*3+3),z=Math.max(-.2,(g[2]-15)/220)*envelope,size=V==='B'?1/(1+z):1+z;
  const [ax,ay]=jack?(skillSkinId==='0'?[left?22:98,66]:skillSkinId==='1'?[left?25:95,70.5]:[left?27:93,64.5]):frog?[left?22:98,66]:cfg.anchors[i],R=rest[5+i],G=tr[5+i],unit=rest[2]/(skillCharacter==='batalhator'&&s.native?.stage===3?120:cfg.w),vunit=rest[3]/cfg.h;
  let scale=Math.min(1.8,size*bodyScale),x=(g[0]-(i?35:-35))*.5*sx*envelope*direction+dx+(ax-cfg.w*.5)*(bodyScale-1),y=-(g[1]-baseY)*.5*sy*envelope+dy+(ay-cfg.h*.55)*(bodyScale-1);
  if(capture){
   const F=capture.frame.hands[i],R=capture.rest.hands[i],r=capture.rest.screen;
   const mirror=jack||frog?1:Math.sign((cfg.anchors[1][0]-cfg.anchors[0][0])*(capture.rest.hands[1][0]-capture.rest.hands[0][0]))||1;
   x=(F[0]-R[0])*cfg.w/r[2]*mirror;y=(F[1]-F[2]/2-R[1]+R[2]/2)*cfg.h/r[3];scale=F[2]/R[2];
  }
  const handMirror=skillCharacter==='vivaldi'&&skillSkinId!=='original'&&V==='B'?-1:1;
  G[0]=R[0]+x*unit/screen/handMirror;G[2]=R[2]*scale;G[1]=R[1]+y*vunit+(G[2]-R[2])/2;G[3]=0;G[5]=V==='B'&&z>.06?0:1;
  centers.push([ax+x,ay+y]);handDelta.push([scale,0,0,scale,ax+x-scale*ax,ay+y-scale*ay]);
 }
 const clips={...data.clips};
 for(const [key,idle]of Object.entries(data.clips).filter(([k])=>k.endsWith('/'+V+'/idle'))){
  const f=idle.frames[0],instances=f.instances.filter(i=>!(skillCharacter==='jack'&&['gL','gR'].includes(i[8]))).map(inst=>{const m=spMul(['gL','gR'].includes(inst[8])?handDelta[inst[8]==='gR'?1:0]:body,inst.slice(1,7));return [inst[0],...m,inst[7],inst[8]];});
  if(!(skillCharacter==='jack'&&skillSkinId==='0'))for(const slot of ['torso','head','hair','hat','cape','tail','legL','legR'])if(!instances.some(i=>i[8]===slot))instances.push([0,...body,1,slot]);
  const frame={...f,opacity:1,instances};clips[key.slice(0,-4)+s.action]={duration:s.duration,loop:false,frames:[frame,frame]};
 }
 return {data:{...data,clips,trajectories:{...data.trajectories,[V+'_'+s.action]:[tr,tr]}},centers,body,S};
}
return o=>{
 if(o.action==='spin'&&o.combat)return angleStudy(o);
 if(o.action==='eject'&&o.combat?.ejection)return koRender(o);
 if(!o.action?.startsWith('skill_'))return skillBase(o);
 const s=o.combat?.skills?.find(s=>s.action===o.action);
 if(!s)return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-40 -45 200 200" data-combat="${o.action}" data-unavailable="true"><text x="60" y="60" text-anchor="middle" fill="#b7c3ce" font-size="6">Habilidade de outro personagem.</text></svg>`;
 if(skillCharacter==='jack'&&!s.native)return jackSpecial(o,s);
 const total=o.actionDuration||s.previewDuration||s.duration+.65,t=Math.max(0,o.time||0)%total,sequence=s.native?.views[o.view==='back'?'back':'front'],frame=sequence?.frames[Math.min(sequence.frames.length-1,Math.floor(t*60+1e-7))],rawCapture=frame?{frame,rest:sequence.rest}:null,capture=s.key==='joana_claw_crush'?joanaStableCrushCapture(s,o.view,rawCapture,t):rawCapture;
 const casting=frame?frame.state===sequence.frames[0].state:t<s.duration,p=frame?casting?spClamp(1-frame.timer/frame.max):1:spClamp(t/s.duration),cfg=spConfig({...o,phase:s.native?.stage||o.phase}),native=spNativeData(o,s,p,cfg,capture);
 const state={...o,time:t,combat:native.data,actionDuration:total,specialPhase:p,specialCasting:casting},change=spEase((p-s.at)/.12);
 if(skillCharacter==='batalhator')state.phase=s.native?.stage||o.phase;
 if(s.key==='vivaldi_four_seasons_mode'&&o.season==='concert'&&p>=s.at)state.season=['spring','summer','autumn','winter'][Math.floor((30+Math.floor(t*60))/90)%4];
 if(skillCharacter==='jack'){
  const key=s.key;
  if(/molasses/.test(key)&&p>=.35)state.state=o.state==='pele_de_mel'||o.state==='ambos'?'ambos':'modo_melaco';
  if(/royal_jelly/.test(key)&&p>=.30)state.state=skillSkinIndex===0?'geleia_real':o.state==='modo_melaco'?'ambos':'pele_de_mel';
 }
 if(skillCharacter==='frog'&&p<1){
  const stages=s.key==='frog_tongue'?[[.08,0],[.16,1],[.24,2],[.32,3],[.68,4],[.78,5],[.88,6]]:s.key==='frog_royal_leap'?[[.35,0],[.48,'no_crown'],[.68,'squash_no_crown'],[.84,'no_crown']]:s.key==='frog_mud_slime'?[[.22,'inflated'],[.32,2],[.44,3],[.72,'mud_spit'],[.84,5],[.92,6]]:s.key==='frog_fly_catcher'?[[.14,1],[.26,2],[.38,3],[.68,4],[.82,'inflated'],[.92,5],[1,'charge']]:s.key==='frog_swamp_spout'?[[.25,'charge'],[.38,3],[.68,4],[.82,5]]:[];
  state.specialMouthStage=stages.find(([end])=>p<end)?.[1]||0;
  if(frame)state.specialMouthStage=/^[0-6]$/.test(frame.mouth)?+frame.mouth:frame.mouth;
 }
 if(skillCharacter==='joana'){
  state.specialHookThrown=s.key==='joana_hook_pull'&&(frame?frame.hands[0][6].includes('WristSocket'):p>=.22&&p<=.85);
  if(s.key==='joana_claw_crush'&&p<1)state.specialClaw=p<.16?-.3:p<.32?-.65:p<.50?-1:p<.54?-1+3*spEase((p-.50)/.04):p<.66?2:2*(1-spEase((p-.66)/.16));
  else if(s.key==='joana_ocean_bounty'&&p>=.18&&p<.78)state.specialClaw=-.6*Math.max(0,Math.sin((p-.18)/.60*Math.PI*6));
  if(frame&&['joana_claw_crush','joana_ocean_bounty'].includes(s.key))state.specialClaw=({open1:-.3,open2:-.65,open3:-1,clamp:2,closed:0})[frame.claw.toLowerCase()]??0;
  if(s.key==='joana_claw_crush'){
   state.specialNativeClaw=state.specialClaw;
   state.specialClaw=joanaCrushMotion(s,o.view,t).jaw;
  }
  joanaAttachedSpecial(state,s,t,native,total);
 }
 if(skillCharacter==='cremoso'&&s.key==='cremoso_vanilla_whip')state.specialVortex=spEase(p/.10)*(1-spEase((p-.88)/.12));
 if(skillCharacter==='cremoso'){
  state.specialMouthStage=frame?frame.mouth:s.key==='cremoso_cherry_topping'&&p<1?p<.16?'Inflated1':p<.32?'Inflated2':p<.48?'Inflated':p<.78?'CherrySpit':p<.90?'Inflated1':'':undefined;
  state.specialBoiling=s.key==='cremoso_syrup_blast'&&p<1;
  if(s.key==='cremoso_cone_punch')state.cone=false;
  if(s.key==='cremoso_puddle_trap'){
   state.liquidAmount=spEase((t-.035)/.42)*(1-spEase((t-(total-.49))/.43));
   state.puddleAmount=state.liquidAmount;
   if(state.liquidAmount>.001){state.fluidOriginalMode=o.mode;state.mode='melt';}
  }
 }
 const original=spInner(skillBase(state));let artwork=original;
 if(s.key==='frog_poison_skin'||s.key==='cremoso_melt_mode'){
  if(s.key==='frog_poison_skin')state.state='poison';else state.mode='melt';
  artwork=`${change<1?`<g opacity="${spN(1-change)}">${original}</g>`:''}<g opacity="${spN(change)}">${spInner(skillBase(state))}</g>`;
 }
 if(s.key==='batalhator_dual_saw_vortex'){
  const release=frame?(frame.hands.some(h=>/WristSocket/i.test(h[6]))?1:0):spEase(p/.08)*(1-spEase((p-.90)/.10));
  if(release)artwork=spInner(skillBase({...state,armed:false}));
 }
 const clone=s.key==='batalhator_holo_combo'?spInner(skillBase({...o,phase:3,combat:o.combat,action:'idle',time:0})):'';
 const options={...o,specialMouthStage:state.specialMouthStage,specialBodyY:native.body[5],specialClone:clone};
 let specialParts=null;
 if(['frog_royal_leap','cremoso_vanilla_whip','batalhator_dual_saw_vortex','batalhator_holo_combo','marcov_orbital_laser','marcov_plasma_barrier'].includes(s.key)||skillCharacter==='marcov'&&[1,3].includes(skillSkinIndex)){
  const key=[s.key,o.view,o.state,o.mode,state.phase].join('/');
  if(!spPropsCache.has(key)){const parts=skillBase({...state,action:'idle',time:0,exportSpecialParts:true});if(s.key==='batalhator_holo_combo')parts.clone=spInner(skillBase({...state,phase:3,action:'idle',time:0}));spPropsCache.set(key,parts);}
  specialParts=spPropsCache.get(key);
 }
 if(skillCharacter==='marcov'&&skillSkinIndex===3&&specialParts?.orbital)specialParts={...specialParts,orbital:specialParts.orbital.replace(/(data-propeller="true" transform=")rotate\([^)]*\)/,`$1rotate(${spN(t*1800%360)})`)};
 const revision=revisedSpecialEffects({...o,specialParts},s,t,cfg,capture,native,total);
 const recorded=revision?null:spRecordedEffects({...o,specialParts,specialMouthStage:state.specialMouthStage,specialBodyMatrix:native.body,specialHandCenters:native.centers},s,t,cfg,capture);
 const jackFX=skillCharacter==='jack'&&!recorded?jackSpecialEffects(options,s,p,t,native.centers):null;
 const fx=revision||recorded||(jackFX?{under:jackFX.behind,over:jackFX.front}:axialSpecialEffects(s,p,t,options,native.centers,cfg)),scale=recorded||revision?'':`scale(${cfg.w/120} ${cfg.h/120})`;
 if(recorded?.replacement&&frame?.override)artwork=recorded.replacement;
 const jump=/royal_leap|lucha_flyer/.test(s.key),tall=/swamp_spout|volcanic_erupt|orbital_apocalypse/.test(s.key),tongue=/frog_tongue/.test(s.key),sky=/marcov_orbital_laser/.test(s.key);
 const crush=s.key==='joana_claw_crush'?joanaCrushMotion(s,o.view,t):null,impact=crush?` data-claw-contact-time="${spN(crush.snap)}" data-camera-shake="${crush.shake.join(' ')}" data-camera-retimed="claw-contact"`:'';
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${-cfg.w*(tongue?1.25:.55)} ${-cfg.h*(jump?1.8:sky?1.5:tall?.90:.45)} ${cfg.w*(tongue?3.5:2.2)} ${cfg.h*(sky?3.7:tongue?3.2:jump?3.4:tall?2.9:2.6)}" data-combat="${o.action}" data-special="${s.key}" data-special-phase="${spN(p)}" data-special-season="${state.season||''}" data-native-frame="${Math.floor(t*60+1e-7)}" data-camera="${o.view==='back'?'back':'front'}-axial" data-pose-source="${s.poseSource}" data-effects="${revision?'requested-skin-redesign':recorded?'native-cpp-capture':'axial-themed-svg-adaptation'}"${impact}><g transform="${scale}">${fx.under}</g>${artwork}<g transform="${scale}">${fx.overlay||''}${fx.over}</g></svg>`;
};

})();
