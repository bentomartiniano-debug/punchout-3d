(window.KR=window.KR||{})["frog/3"]=(function(){
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

const skillBase=(()=>{const makeArt=()=>{// Shared by the SVG exporter and the laboratory. Cosmetic only: no gameplay data.
function refinedFrogGlove(skin, right, poison = false, view = 'front') {
  const ink = '#101b17';
  const metal = '#b87536';
  const light = '#f5ce84';
  const back = view === 'back';
  let art = '';
  if (skin === 1) {
    art = `<rect x="-12" y="-14" width="24" height="28" rx="6" fill="#34343d" stroke="${ink}" stroke-width="2"/>
      <path d="M -9 -8 Q -7 -13 2 -11" fill="none" stroke="#73737e" stroke-width="2" stroke-linecap="round"/>
      <rect x="2" y="-4" width="12" height="14" rx="4" fill="#24242d" stroke="${ink}" stroke-width="2"/>
      <path d="M -9 4 H 1" fill="none" stroke="#535360" stroke-width="0.8"/>
      <rect x="-10" y="8" width="20" height="5" rx="1" fill="#20212b" stroke="${ink}" stroke-width="1"/>
      <path d="M -9 10 H 9" stroke="#a23c48" stroke-width="1"/>
      ${back ? '<path d="M -7 -3 V 4 M -3 -3 V 4" stroke="#535360" stroke-width="0.8"/>' : '<path d="M -8 -5 Q -3 -7 1 -5" fill="none" stroke="#535360" stroke-width="0.8"/>'}`;
  } else if (skin === 2) {
    art = `<rect x="-12" y="-14" width="24" height="28" rx="6" fill="#633c29" stroke="${ink}" stroke-width="2"/>
      <rect x="2" y="-4" width="12" height="14" rx="4" fill="#835237" stroke="${ink}" stroke-width="2"/>
      <path d="M -10 -11 H 6 L 8 -4 H -10 Z" fill="${metal}" stroke="${ink}" stroke-width="1.3"/>
      <path d="M -8 -10 H 4" stroke="${light}" stroke-width="1.8" stroke-linecap="round"/>
      <circle cx="-7" cy="-6" r="1" fill="${light}"/><circle cx="4" cy="-6" r="1" fill="${light}"/>
      <rect x="-10" y="7" width="20" height="7" rx="1" fill="#e9dbb6" stroke="${ink}" stroke-width="1.2"/>
      <path d="M -10 8 L 9 12 M -9 12 L 9 8" stroke="#a99b7d" stroke-width="1"/>
      ${back ? '<path d="M -7 0 L 1 5 M 1 0 L -7 5" stroke="#ae8560" stroke-width="1.4"/>' : ''}`;
  } else {
    art = `<rect x="-12" y="-14" width="24" height="28" rx="6" fill="#facc15" stroke="${ink}" stroke-width="2"/>
      <rect x="2" y="-4" width="12" height="14" rx="4" fill="#dcae13" stroke="${ink}" stroke-width="2"/>
      <path d="M -10 -8 Q -4 -13 0 -7 Q 3 -2 -3 1 Q -11 1 -10 -8 Z" fill="#121c30"/>
      <path d="M -8 -12 H -2" stroke="#fff6b0" stroke-width="2" stroke-linecap="round"/>
      <rect x="-10" y="8" width="20" height="5" rx="1" fill="#0b634e" stroke="${ink}" stroke-width="1"/>
      <path d="M -7 10 H 3" stroke="#75d7b0" stroke-width="1"/>
      <path d="M 5 8 L 8 10.5 L 5 13 L 2 10.5 Z" fill="${poison ? '#b7ffd2' : '#2ac793'}" stroke="${ink}" stroke-width="0.7"/>
      ${poison ? '<path class="frog-pulse" d="M -9 3 H -5 V 6 H 0" fill="none" stroke="#8affad" stroke-width="1.2"/>' : ''}`;
  }
  return `<g transform="translate(${right ? 98 : 22},66) scale(0.38) ${right ? 'scale(-1,1)' : ''} translate(-60,-60)"><g transform="translate(60,60) scale(2.5)">${art}</g></g>`;
}

function frogVocalSac(skin, stage, poison) {
  if (!stage) return '';
  const rx = stage<=1?20*stage:16+stage*4;
  const ry = stage<=1?5*stage:2+stage*3;
  const y = 53 - stage * 0.8;
  const tone = skin === 3 ? '#facc15' : skin === 1 ? '#b9cba5' : '#cfe1ad';
  const fill = poison ? (skin === 3 ? '#d8ef40' : '#559263') : tone;
  return `<g data-papo-stage="${stage}">
    <ellipse cx="60" cy="${y}" rx="${rx}" ry="${ry}" fill="${fill}" stroke="#101b17" stroke-width="2"/>
    <path d="M ${60-rx*0.7} ${y+1} Q 60 ${y+ry*0.9} ${60+rx*0.7} ${y+1}" fill="none" stroke="${poison ? '#345e3e' : '#879b68'}" stroke-width="1.4"/>
    <ellipse cx="${60-rx*0.32}" cy="${y-ry*0.32}" rx="${rx*0.18}" ry="${Math.max(1,ry*0.15)}" fill="#fffde6" opacity="0.55"/>
    ${skin === 3 ? '<path d="M 37 52 Q 39 56 42 54 M 80 51 Q 85 54 82 57" fill="none" stroke="#151d2e" stroke-width="2"/>' : ''}
  </g>`;
}

function frogFeathers(poison, back) {
  // Outer pairs first, central plume last: a symmetrical fan, not a sideways stack.
  const angles = [-64, 64, -43, 43, -22, 22, 0];
  const plumes = angles.map(a => `<g transform="rotate(${a},60,25)">
    <path d="M 57 25 Q 49 8 60 2 Q 71 8 63 25 Z" fill="${Math.abs(a)===43 ? '#1685bc' : '#139873'}" stroke="#101b17" stroke-width="1.4"/>
    <path d="M 60 7 V 23" stroke="${Math.abs(a)===43 ? '#70c9d8' : '#66d1a6'}" stroke-width="1.2"/>
  </g>`).join('');
  return `<g>${plumes}
    <path d="M 38 25 Q 60 19 82 25 L 81 29 Q 60 23 39 29 Z" fill="#e6b72b" stroke="#101b17" stroke-width="1.6"/>
    ${back ? '<path d="M 55 27 L 50 41 L 57 37 L 60 28 L 64 42 L 69 39 L 65 26" fill="#096e59" stroke="#101b17" stroke-width="1.2"/>' : `<path d="M 60 18 L 65 23 L 60 29 L 55 23 Z" fill="${poison ? '#a4ffca' : '#29c495'}" stroke="#101b17" stroke-width="1.6"/><path d="M 60 20 L 60 25 L 57 23 Z" fill="#a1f2cd"/>`}
  </g>`;
}

function finishFrogParts(p) {
  let { skin, stage, poison, bare, view, expression, P, ink } = p;
  const back = view === 'back';
  const line = 'stroke="#101b17" stroke-width="2" stroke-linejoin="round"';
  // Poison affects the frog's skin, never bronze, gold or fabric.
  const metal = '#b87536';
  const shadow = '#75482d';
  const bright = '#f5ce84';
  const gold = '#e9c44e';
  const sac = frogVocalSac(skin, stage, poison);
  p.background = ''; p.foreground = '';
  p.style = `<style>
    .frog-smoke { animation: frog-smoke-rise 2.8s ease-out infinite; transform-box: fill-box; transform-origin:center; }
    .frog-smoke.slow { animation-delay: -1.4s; }
    .frog-pulse { animation: frog-rune-glow 1.8s ease-in-out infinite alternate; }
    .frog-drift { animation: frog-drift 3s ease-in-out infinite alternate; }
    @keyframes frog-smoke-rise { 0% { opacity:.7; transform:translateY(2px) scale(.75); } 100% { opacity:0; transform:translateY(-7px) scale(1.25); } }
    @keyframes frog-rune-glow { from { opacity:.45; } to { opacity:1; } }
    @keyframes frog-drift { from { transform:translateY(1px); } to { transform:translateY(-2px); } }
    @media (prefers-reduced-motion:reduce) { .frog-smoke,.frog-pulse,.frog-drift { animation:none; } }
    .frog-still .frog-smoke,.frog-still .frog-pulse,.frog-still .frog-drift { animation:none; }
  </style>`;
  p.inflatedSvg = '';
  if (skin === 1) {
    // Both lapels, the shirt, bow tie and carnation travel together. Their
    // opening and downward motion follow each stage, rather than snapping.
    const garmentDrop = 7 + stage * 2;
    const waistDrop = stage * 1.25;
    const leftTip = 55 - stage * 4.75;
    const rightTip = 65 + stage * 4.75;
    const shirt = `<path d="M 48 46 H 72 L 66 68 H 54 Z" fill="#f4f0e6" ${line}/>
      <path d="M 50 48 L 59 52 L 60 49 L 61 52 L 70 48" fill="none" stroke="#b8b5b1" stroke-width="1.1"/>`;
    const lapels = `<path d="M 35 45 L 48 46 L ${leftTip} 66 L 41 70 Z" fill="#44434d" stroke="#101b17" stroke-width="1.3"/>
      <path d="M 85 45 L 72 46 L ${rightTip} 66 L 79 70 Z" fill="#44434d" stroke="#101b17" stroke-width="1.3"/>
      <path d="M 37 46 L 45 47 M 83 46 L 75 47" stroke="#72727e" stroke-width="1.1"/>
      <path d="M 51 49 L 59 52 L 51 55 Z M 69 49 L 61 52 L 69 55 Z" fill="#20212a" stroke="#101b17" stroke-width="1"/>
      <circle cx="60" cy="52" r="1.4" fill="#535360"/>
      <path d="M 77 55 L 75 60" stroke="#46724f" stroke-width="1.1"/>
      <path d="M 74 51 Q 71 52 73 54 Q 71 57 75 58 Q 78 59 79 56 Q 82 54 79 52 Q 77 49 74 51 Z" fill="#d44b57" stroke="#101b17" stroke-width="1"/>
      <path d="M 74 54 Q 76 52 78 54" fill="none" stroke="#ffb0a2" stroke-width="1"/>`;
    p.torsoSvg = `<rect x="28" y="35" width="64" height="50" rx="10" fill="#30303b" ${line}/>
      ${[34,42,78,86].map(x=>`<path d="M ${x} 46 V 72" stroke="#60616c" stroke-width="0.65"/>`).join('')}
      <path d="M 34 43 Q 60 39 86 43 V 53 Q 60 58 34 53 Z" fill="${P.skinTone}"/>
      <g data-sapone-shirt="${stage}" transform="translate(0,${garmentDrop})">${shirt}</g>${sac}
      <g data-sapone-lapels="${stage}" transform="translate(0,${garmentDrop})">${lapels}</g>
      ${stage ? '' : '<circle cx="60" cy="68" r="1.3" fill="#dbc081"/>'}
      <g data-sapone-waist="${stage}" transform="translate(0,${waistDrop})">
      <path d="M 28 73 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="#24242e" ${line}/>
      <path d="M 34 79 V 88 M 86 79 V 88" stroke="#60616c" stroke-width="0.7"/>
      <rect x="28" y="73" width="64" height="5" fill="#141820"/>
      <rect x="52" y="71" width="16" height="9" rx="2" fill="#c9c9ca" ${line}/><rect x="56" y="74" width="8" height="3" fill="#23232b"/>
      </g>`;
    p.headwearSvg = p.headwearSvg.replace('#991b1b','#754a9e').replace('fill="#18181b"','fill="#30303b"').replace('fill="#27272a"','fill="#44434d"');
    // The wide frog cheeks are skin, not the upper corners of the suit.
    // Keep them with the moving head, above the costume, in every expression.
    p.headDomeSvg = `<path data-sapone-cheeks="skin" d="M 36 32 Q 28 32 28 43 Q 26 50 36 53 H 84 Q 94 50 92 43 Q 92 32 84 32 Z" fill="${P.skinTone}" stroke="#101b17" stroke-width="2"/>` + p.headDomeSvg;
    p.stumpsSvg = p.stumpsSvg.replaceAll(P.skinTone,'#45434e');
    p.faceSvg += expression === 'normal' && !stage ? '<g fill="none" stroke="#cdd9d4" stroke-width="0.8"><ellipse class="frog-smoke" cx="103" cy="34" rx="3.2" ry="1.7"/><ellipse class="frog-smoke slow" cx="106" cy="27" rx="4" ry="2"/></g>' : '';
  } else if (skin === 2) {
    p.torsoSvg = `<rect x="28" y="35" width="64" height="50" rx="10" fill="${P.skinTone}" ${line}/>${sac}
      <g data-roman-shoulder="left" transform="translate(-10,4)">
      <path d="M 27 39 Q 33 34 42 37 L 46 48 L 39 57 L 26 52 Z" fill="${metal}" ${line}/>
      <path d="M 29 39 Q 35 37 41 39 L 43 44 L 28 46 Z" fill="${bright}"/>
      <path d="M 28 48 L 41 46" stroke="${shadow}" stroke-width="1.6"/>
      </g>
      <g data-roman-breastplate="${stage}" transform="translate(0,${5+stage*0.6})">
        <path d="M 37 47 Q 60 51 83 47 L 81 67 Q 60 73 39 67 Z" fill="${metal}" ${line}/>
        <path d="M 39 49 Q 48 53 58 51 L 56 57 Q 46 58 39 54 Z M 62 51 Q 72 53 81 49 L 79 54 Q 72 58 64 57 Z" fill="${bright}"/>
        <path d="M 40 59 Q 49 62 57 60 M 63 60 Q 72 62 80 59 M 60 53 V 66" fill="none" stroke="${shadow}" stroke-width="1.4"/>
        <path d="M 42 66 Q 60 71 78 66" fill="none" stroke="${bright}" stroke-width="1.3"/>
      </g>
      <path d="M 28 74 Q 55 78 92 73 V 86 L 75 88 L 58 82 L 43 89 L 28 85 Z" fill="#673384" ${line}/>
      <path d="M 37 77 L 35 84 M 50 78 L 46 86 M 67 77 L 70 83 M 84 77 L 87 84" stroke="#43224f" stroke-width="2"/>
      <path d="M 29 84 L 42 87 L 58 80 L 75 86 L 91 84" fill="none" stroke="#e4bf60" stroke-width="1.3"/>
      <path d="M 31 87 V 90 H 39 V 88 M 81 88 V 90 H 89 V 86" fill="#70422d" stroke="#101b17" stroke-width="1.4"/>
      <rect x="28" y="74" width="64" height="5" fill="#65422e" stroke="#101b17" stroke-width="1.3"/>
      <rect x="53" y="72" width="14" height="9" rx="2" fill="${metal}" ${line}/><circle cx="60" cy="76.5" r="2.4" fill="${bright}"/>`;
    p.headwearSvg = p.headwearSvg.replaceAll('#facc15',gold).replaceAll('#fbbf24',bright).replaceAll('#ca8a04',shadow).replaceAll('#b45309',metal);
    // The hanging purple ribbons sit on the head, behind its facial features.
    // The gold laurel leaves keep their independent foreground layer.
    const ribbons = [];
    p.headwearSvg = p.headwearSvg.replace(/<path d="M (?:44 26 Q 38 34 40 42|76 26 Q 82 34 80 42)"[^>]*\/>/g, ribbon => { ribbons.push(ribbon); return ''; });
    if (ribbons.length) p.faceSvg = `<g data-roman-ribbons="behind-eyes">${ribbons.join('')}</g>` + p.faceSvg;
    p.legsSvg = p.legsSvg.replaceAll('#b45309',metal).replaceAll('#fbbf24',bright);
  } else if (skin === 3) {
    p.torsoSvg = `<rect x="28" y="35" width="64" height="50" rx="10" fill="${P.skinTone}" ${line}/>
      <path d="M 29 45 Q 39 42 38 51 Q 34 59 29 55 Z M 91 45 Q 81 42 82 51 Q 86 59 91 55 Z" fill="#0d172a"/>
      <path d="M 31 60 Q 38 58 36 66 L 29 69 Z M 89 60 Q 82 58 84 66 L 91 69 Z" fill="#0d172a"/>
      <path d="M 38 51 Q 60 56 82 51 L 82 65 Q 60 78 38 65 Z" fill="#facc15" ${line}/>${sac}
      <path d="M 28 73 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="#172338" ${line}/>
      <path d="M 30 85 H 56 M 64 85 H 90" stroke="#e2bb23" stroke-width="2"/>
      <path d="M 32 79 H 42 V 83 H 49 L 51 80 L 47 77 M 88 79 H 78 V 83 H 71 L 69 80 L 73 77" fill="none" stroke="#59c4a4" stroke-width="1.5" stroke-linejoin="miter"/>
      <path d="M 35 77 L 38 75 M 40 78 L 43 76 M 85 77 L 82 75 M 80 78 L 77 76" stroke="#e4bd32" stroke-width="1"/>
      <rect x="28" y="72" width="64" height="5" fill="#09654f"/>
      <path d="M 60 68 L 67 74 L 60 80 L 53 74 Z" fill="${poison ? '#a0ffc3' : '#23bd91'}" ${line}/>
      <path d="M 60 70 V 76 L 56 74 Z" fill="#91e9c3"/>`;
    p.headwearSvg = bare ? '' : frogFeathers(poison, false);
    p.headDomeSvg += `<path d="M 34 28 Q 39 25 40 29 L 37 31 Z M 86 28 Q 81 25 80 29 L 83 31 Z" fill="#10192c"/>`;
    p.faceSvg = `<path d="M 33 37 L 37 39 L 34 43 M 87 37 L 83 39 L 86 43" fill="none" stroke="#f5eddb" stroke-width="1.8"/>
      <path d="M 36 39 L 40 41 M 84 39 L 80 41" stroke="#d9524a" stroke-width="1.8"/>
      <path d="M 58 29 L 60 32 L 62 29" fill="none" stroke="#f5eddb" stroke-width="1.2"/>${p.faceSvg}`;
    p.legsSvg += '<path d="M 36 91 Q 43 87 46 91 L 44 95 L 38 94 Z M 74 97 Q 78 91 83 95 L 82 100 L 77 100 Z" fill="#10192c"/>';
  } else if (stage) {
    // Preserve the canonical uninflated sprite; replace only its inflation overlay.
    p.inflatedSvg = sac;
  }

  if (back) {
    const bodyFill = skin === 1 ? '#30303b' : P.skinTone;
    const body = `<path d="M 28 42 Q 28 35 38 35 H 82 Q 92 35 92 42 V 75 H 28 Z" fill="${bodyFill}" ${line}/>`;
    const expanded = stage ? `<path d="M 29 46 Q ${24-stage} 54 29 65 M 91 46 Q ${96+stage} 54 91 65" fill="${bodyFill}" stroke="#101b17" stroke-width="2"/><path d="M 40 49 Q 60 ${51+stage} 80 49" fill="none" stroke="${skin === 1 ? '#666571' : P.skinDark}" stroke-width="1.2"/>` : '';
    let costume = '';
    if (skin === 1) {
      costume = `${[34,42,50,70,78,86].map(x=>`<path d="M ${x} 45 V 73" stroke="#60616c" stroke-width="0.7"/>`).join('')}
        <path d="M 35 47 Q 60 53 85 47 M 60 54 V 82" fill="none" stroke="#181b24" stroke-width="1.6"/>
        <path d="M 29 74 H 91 V 90 H 63 L 60 84 L 57 90 H 29 Z" fill="#24242e" ${line}/><path d="M 60 77 V 85" stroke="#62626d" stroke-width="1"/>
        <rect x="29" y="74" width="62" height="4" fill="#151821"/>`;
    } else if (skin === 2) {
      costume = `<g data-roman-straps="full-back">
        <path d="M 29 42 L 91 76 M 91 42 L 29 76" stroke="#101b17" stroke-width="6"/>
        <path d="M 29 42 L 91 76 M 91 42 L 29 76" stroke="#775139" stroke-width="3.5"/>
        <circle cx="60" cy="59" r="3" fill="${metal}" stroke="#101b17" stroke-width="1.3"/>
        </g>
        <g data-roman-shoulder="left" transform="translate(-10,4)"><path d="M 28 39 Q 34 34 43 38 L 44 52 L 28 54 Z" fill="${metal}" ${line}/><path d="M 30 40 L 41 39 L 41 43 L 29 46 Z" fill="${bright}"/></g>
        <path d="M 28 74 Q 57 78 92 74 V 87 L 75 90 L 60 85 L 42 90 L 28 87 Z" fill="#673384" ${line}/>
        <path d="M 38 78 L 36 85 M 52 79 L 48 87 M 68 79 L 72 87 M 83 78 L 86 85" stroke="#43224f" stroke-width="2"/>
        <path d="M 30 85 L 42 88 L 60 83 L 75 88 L 90 85" fill="none" stroke="#e4bf60" stroke-width="1.3"/>
        <rect x="28" y="74" width="64" height="4" fill="#65422e" stroke="#101b17" stroke-width="1.2"/>`;
    } else if (skin === 3) {
      costume = `<path d="M 33 45 Q 36 40 42 44 L 42 51 Q 35 56 33 45 Z M 78 43 Q 86 41 86 49 L 81 54 L 77 48 Z M 53 50 Q 62 46 67 52 Q 69 60 59 60 Q 51 58 53 50 Z M 35 60 Q 43 57 46 64 L 41 69 L 34 67 Z M 75 61 Q 81 59 85 64 L 82 69 L 74 68 Z" fill="#10192c"/>
        <path d="M 28 73 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="#172338" ${line}/>
        <path d="M 32 85 H 55 M 65 85 H 88" stroke="#e2bb23" stroke-width="2"/>
        <path d="M 34 79 H 45 V 82 H 51 M 86 79 H 75 V 82 H 69" fill="none" stroke="#59c4a4" stroke-width="1.5"/>
        <rect x="28" y="72" width="64" height="5" fill="#09654f"/>`;
    } else {
      costume = `<path d="M 28 73 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="#2b4528" ${line}/><path d="M 29 86 H 57 M 63 86 H 91" stroke="#e5cc5b" stroke-width="2"/><rect x="28" y="72" width="64" height="5" fill="#0f1c10"/>`;
    }
    p.torsoSvg = body + expanded + costume;
    p.inflatedSvg = '';
    p.headDomeSvg = `<path d="M 34 43 V 34 C 34 21 52 20 54 30 Q 60 27 66 30 C 68 20 86 21 86 34 V 43 Q 60 53 34 43 Z" fill="${P.skinTone}" stroke="#101b17" stroke-width="2"/>
      <path d="M 42 44 Q 60 48 78 44" fill="none" stroke="${P.skinDark}" stroke-width="1.2"/>${bare ? '<ellipse cx="59" cy="29" rx="5" ry="1.7" fill="#f6ffe1" opacity=".4"/>' : ''}`;
    p.faceSvg = ''; p.nostrilsSvg = '';
    if (skin === 1) {
      // Back of the shoes: heel panels, not the front spats and buttons.
      p.legsSvg = `<path d="M 35 85 V 105 H 48 V 85 Z M 72 85 V 105 H 85 V 85 Z" fill="#24242e" ${line}/>
        <path d="M 42 88 V 102 M 78 88 V 102" stroke="#60616c" stroke-width=".8"/>
        <path d="M 17 103 H 48 V 110 H 17 Z M 72 103 H 103 V 110 H 72 Z" fill="#20242c" ${line}/>
        <path d="M 39 103 H 47 V 106 H 39 Z M 73 103 H 81 V 106 H 73 Z" fill="#d0cec8"/>
        <path d="M 18 108 H 47 M 73 108 H 102" stroke="#5a5c65" stroke-width="1"/>`;
    } else if (skin === 2 && !bare) {
      p.headwearSvg = `<path d="M 38 27 Q 60 18 82 27" fill="none" stroke="${shadow}" stroke-width="2"/>
        ${[40,47,54,66,73,80].map((x,i)=>`<ellipse cx="${x}" cy="${i===0||i===5 ? 25 : i===1||i===4 ? 22 : 20}" rx="4" ry="2.2" fill="${gold}" stroke="#101b17" stroke-width="1.1" transform="rotate(${x<60?-20:20},${x},${i===0||i===5 ? 25 : i===1||i===4 ? 22 : 20})"/>`).join('')}
        <path d="M 58 25 L 52 40 L 58 38 L 61 28 L 64 41 L 69 38 L 64 25" fill="#673384" stroke="#101b17" stroke-width="1.1"/>`;
    } else if (skin === 0) {
      p.headwearSvg = p.headwearSvg.replace(/<rect x="58"[^>]*\/>\s*<rect x="59"[^>]*\/>/, '');
    }
    if (skin === 3) {
      p.headDomeSvg += '<path d="M 38 32 Q 45 28 46 33 L 43 38 L 38 36 Z M 74 32 Q 81 28 82 33 L 79 38 L 74 36 Z" fill="#10192c"/>';
      p.headwearSvg = bare ? '' : frogFeathers(poison, true);
    }
  }

  enhanceFrogParts(p);
  if (stage && !back) {
    const move = s => `<g transform="translate(0,${-stage*1.65})">${s}</g>`;
    p.headDomeSvg = move(p.headDomeSvg); p.nostrilsSvg = move(p.nostrilsSvg);
    p.faceSvg = move(p.faceSvg);
    // Flexible plumes compress slightly in the charging pose, keeping every tip
    // within the canonical 120×120 sprite frame at all four inflation stages.
    if (skin === 3 && !bare) p.headwearSvg = `<g transform="translate(0,25) scale(1,${1-stage*0.065}) translate(0,-25)">${p.headwearSvg}</g>`;
    p.headwearSvg = move(p.headwearSvg);
  }
  if (poison && skin > 0) {
    p.toxicSvg = '';
    if (skin === 1) {
      p.background = '<g fill="#a9c3ae" opacity=".2"><path d="M 24 74 Q 8 63 22 48 Q 15 40 28 36 Q 23 53 31 61 Z"/><path d="M 95 72 Q 111 60 98 46 Q 105 39 92 36 Q 97 54 88 61 Z"/></g>';
      p.toxicSvg = `<g fill="none" stroke="#a9d8b4" stroke-width="1.2"><path class="frog-smoke" d="M 26 55 Q 20 48 26 43"/><path class="frog-smoke slow" d="M 94 54 Q 100 48 94 42"/></g>`;
      p.foreground = '<g class="frog-drift"><g transform="rotate(-16,10,46)"><rect x="5" y="42" width="10" height="6" rx="1" fill="#81a58c" stroke="#243c2e" stroke-width=".7"/><ellipse cx="10" cy="45" rx="1.6" ry="2" fill="none" stroke="#375a42" stroke-width=".6"/></g><g transform="rotate(18,110,81)"><rect x="105" y="78" width="10" height="6" rx="1" fill="#81a58c" stroke="#243c2e" stroke-width=".7"/><ellipse cx="110" cy="81" rx="1.6" ry="2" fill="none" stroke="#375a42" stroke-width=".6"/></g></g>';
    } else if (skin === 2) {
      p.background = '<g class="frog-drift" fill="#5bc7a1" opacity=".22"><path d="M 22 79 Q 12 64 25 43 Q 19 66 35 76 Z"/><path d="M 98 78 Q 107 64 95 43 Q 101 65 85 76 Z"/></g>';
      p.toxicSvg = '<g class="frog-pulse" fill="none" stroke="#7bd5ab" stroke-width="1.3"><path d="M 28 50 Q 23 44 28 38 M 92 53 Q 99 47 95 40"/></g>';
    } else {
      p.toxicSvg = `<g class="frog-pulse" fill="none" stroke="#a0ffb3" stroke-width="1.4" stroke-linejoin="miter">
        <path d="M 33 53 H 37 V 59 H 33 V 56 M 87 53 H 83 V 59 H 87 V 56"/>
        <path d="M 44 ${back ? 60 : 63} H 50 V ${back ? 65 : 68} H 47 M 76 ${back ? 60 : 63} H 70 V ${back ? 65 : 68} H 73"/>
      </g>`;
      p.foreground = '<g class="frog-drift" fill="#91ec8a"><circle cx="13" cy="48" r="1.3"/><circle cx="105" cy="39" r="1.2"/><circle cx="110" cy="84" r="1.3"/><circle cx="9" cy="82" r=".9"/><circle cx="18" cy="35" r="1"/></g>';
    }
  }
  return p;
}

// Applied after approved costumes/back layers, before inflation moves the head.
function enhanceFrogParts(p){
 const {skin,stage,poison,view,expression,P}=p,back=view==='back';
 // Floating fists only: remove support-arm stumps in every skin and form.
 p.stumpsSvg='';
 const light=skin===3?(poison?'#397ebe':'#39a8ef'):(poison?'#52725a':skin===1?'#90a577':'#a4bd74');
 const deep=skin===3?(poison?'#10254d':'#173793'):P.skinDark;
 // The old no-headwear export included a standalone white scalp dot.
 if(p.bare)p.headDomeSvg=p.headDomeSvg.replace(/<ellipse cx="(?:60|59)" cy="(?:24|29)"[^>]*\/>/g,'');
 // The original eye dome was a separate, flat-ended patch over the cheeks.
 // Give eyes and muzzle one silhouette, without an internal edge at y=44.
 if(!back){
  const jawY=57+stage*4.5,cheekX=94+stage*1.2;
  p.headDomeSvg=p.headDomeSvg.replace(/<path data-sapone-cheeks="skin"[^>]*\/>/,'');
  p.headDomeSvg=p.headDomeSvg.replace(/<path d="M 34 34 C 34 20, 54 20, 54 32 Q 60 28, 66 32 C 66 20, 86 20, 86 34 V 44 Q 60 50, 34 44 Z"[^>]*\/>/,
   `<path data-frog-head="continuous" data-frog-body-join="open" d="M 28 43 Q 28 35 34 33 C 34 20 54 20 54 32 Q 60 28 66 32 C 66 20 86 20 86 33 Q 92 35 92 43 Q ${cheekX} ${jawY-2} 60 ${jawY} Q ${120-cheekX} ${jawY-2} 28 43 Z" fill="${P.skinTone}"/><path d="M 28 ${jawY-5} Q 26 48 28 43 Q 28 35 34 33 C 34 20 54 20 54 32 Q 60 28 66 32 C 66 20 86 20 86 33 Q 92 35 92 43 Q 94 48 92 ${jawY-5}" fill="none" stroke="${P.ink}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>`);
  p.headDomeSvg=p.headDomeSvg.replace(/<path d="M 34 34 C 34 (?:20|16),[^"]*" fill="none"[^>]*\/>/,'');
  if(stage){
   // Vocal sac belongs to the underside of the mouth, not to a neck ring.
   p.torsoSvg=p.torsoSvg.replace(/<g data-papo-stage="[^"]+">[\s\S]*?<\/g>/,'');
   p.inflatedSvg='';
   const tone=skin===3?(poison?'#d8ef40':'#f5da4c'):(poison?'#559263':skin===1?'#c5d4b1':'#d8e7b9');
   p.style+=`<defs><radialGradient id="frog-vocal-sac" cx=".36" cy=".28" r=".8"><stop stop-color="${skin===3?'#fff2a2':poison?'#9bc999':'#f0f6d7'}"/><stop offset=".6" stop-color="${tone}"/><stop offset="1" stop-color="${poison?'#3c734c':skin===3?'#c8a42a':'#9fb779'}"/></radialGradient></defs>`;
   p.headDomeSvg+=`<g data-frog-sac="under-mouth" data-sac-stage="${stage}"><path d="M 38 48 Q 60 51 82 48 C 87 ${jawY-8} 77 ${jawY-1} 60 ${jawY-1} C 43 ${jawY-1} 33 ${jawY-8} 38 48 Z" fill="url(#frog-vocal-sac)"/><path d="M 43 ${jawY-8} Q 48 ${jawY-4} 54 ${jawY-4}" fill="none" stroke="#fbffe5" stroke-width="1.5" stroke-linecap="round" opacity=".6"/></g>`;
  }
  if(skin>0){
   // Restore only material boundaries, never a neck seam across bare skin.
   const x=skin===1?28:38,width=skin===1?64:44;
   p.style+=`<defs><clipPath id="frog-clothing-join"><rect x="${x}" y="0" width="${width}" height="120"/></clipPath></defs>`;
   p.headDomeSvg+=`<path data-frog-material-edge="${skin===1?'suit':skin===2?'bronze':'yellow-belly'}" clip-path="url(#frog-clothing-join)" d="M 92 43 Q ${cheekX} ${jawY-2} 60 ${jawY} Q ${120-cheekX} ${jawY-2} 28 43" fill="none" stroke="${P.ink}" stroke-width="1.8" stroke-linecap="round"/>`;
  }
 }else{
  // Keep the external eye humps, not the closed helmet-like lower contour.
  p.headDomeSvg=p.headDomeSvg.replace(/(<path d="M 34 43 V 34 C 34 21 52 20 54 30 Q 60 27 66 30 C 68 20 86 21 86 34 V 43 Q 60 53 34 43 Z"[^>]*?) stroke="#101b17" stroke-width="2"\/>/,'$1 data-frog-body-join="open"/>');
  p.headDomeSvg=p.headDomeSvg.replace(/<path d="M 42 44 Q 60 48 78 44"[^>]*\/>/,'');
  p.headDomeSvg+='<path d="M 34 43 V 34 C 34 21 52 20 54 30 Q 60 27 66 30 C 68 20 86 21 86 34 V 43" fill="none" stroke="#101b17" stroke-width="2" stroke-linecap="round"/>';
  if(skin===1)p.headDomeSvg+='<path data-frog-material-edge="suit" d="M 34 43 Q 60 53 86 43" fill="none" stroke="#101b17" stroke-width="1.8" stroke-linecap="round"/>';
  p.torsoSvg=p.torsoSvg.replace(/<path d="M 40 49 Q 60 [\d.]+ 80 49"[^>]*\/>/,'');
 }
 if(skin===0&&!p.bare){
  p.style+='<defs><linearGradient id="frog-polished-gold" x1="0" y1="0" x2="1" y2=".7"><stop stop-color="#ac711d"/><stop offset=".28" stop-color="#ffe88b"/><stop offset=".44" stop-color="#fff8cd"/><stop offset=".53" stop-color="#e6b63d"/><stop offset="1" stop-color="#a56c20"/></linearGradient></defs>';
  p.headwearSvg=`<g data-frog-crown="slender"><path d="M 51 26 Q 52 15 60 15 Q 68 15 69 26 Z" fill="#733980" stroke="${P.ink}" stroke-width="1.5"/><path d="M 50 27 L 48 12 L 55 19 L 60 6 L 65 19 L 72 12 L 70 27 Z" fill="url(#frog-polished-gold)" stroke="${P.ink}" stroke-width="1.65" stroke-linejoin="round"/><path d="M 52 24 H 68" stroke="#fff3b0" stroke-width="1.2" stroke-linecap="round"/><path d="M 50 15 L 51 21 M 60 10 L 58 17" stroke="#fffbd8" stroke-width="1.1" stroke-linecap="round"/>${back?'':`<path d="M 60 19 L 62 21.5 L 60 24 L 58 21.5 Z" fill="#b83b4a" stroke="${P.ink}" stroke-width=".7"/><path d="M 60 20 L 60 21.5 L 59 21.5 Z" fill="#ffa68b"/>`}</g>`;
  p.headwearSvg='<g data-frog-crown-seat="between-eyes" transform="translate(0 5)">'+p.headwearSvg+'</g>';
  if(!back){
   // The crown sits behind the eye humps and moves with the same head layer.
   p.headDomeSvg=p.headwearSvg+p.headDomeSvg;
   p.headwearSvg='';
  }
 }
 const stops=`<stop offset="0" stop-color="${light}"/><stop offset=".42" stop-color="${P.skinTone}"/><stop offset=".76" stop-color="${P.skinTone}"/><stop offset="1" stop-color="${deep}"/>`;
 const ids={head:'frog-skin-head',torso:'frog-skin-body',legs:'frog-skin-legs',stumps:'frog-skin-stumps'};
 p.style+=`<defs>${Object.entries(ids).map(([part,id])=>{const lift=part==='head'&&!back?stage*1.65:0;return `<linearGradient id="${id}" gradientUnits="userSpaceOnUse" x1="30" y1="${part==='legs'?85:22+lift}" x2="90" y2="${part==='legs'?110:80+lift}">${stops}</linearGradient>`;}).join('')}<linearGradient id="frog-belly" x1="0" y1="0" x2=".65" y2="1"><stop stop-color="${skin===3?'#fff19b':P.bellyTone}"/><stop offset=".55" stop-color="${P.bellyTone}"/><stop offset="1" stop-color="${skin===3?'#db9f16':P.bellyDark}"/></linearGradient></defs>`;
 for(const [part,id]of Object.entries(ids)){
  const key=part==='head'?'headDomeSvg':part+'Svg';
  p[key]=p[key].replaceAll(`fill="${P.skinTone}"`,`fill="url(#${id})"`);
 }
 if(skin===0||skin===3)p.torsoSvg=p.torsoSvg.replaceAll(`fill="${P.bellyTone}"`,'fill="url(#frog-belly)"');
 // Small wet reflections; highlights remain underneath eyes, paint and headwear.
 const shine=skin===3?'#c4fff2':'#f0ffe0';
 p.headDomeSvg+=`<g data-frog-wet-skin="head" fill="none" stroke-linecap="round"><path d="${back?'M 39 32 Q 40 27 46 27 M 39 37 L 39 39':'M 36 31 Q 37 27 41 27 M 31 41 Q 30 44 33 45'}" stroke="${shine}" stroke-width="1.65" opacity=".72"/><path d="${back?'M 73 28 Q 77 29 78 32':'M 78 28 L 81 30'}" stroke="${shine}" stroke-width="1.1" opacity=".32"/></g>`;
 if(back&&skin!==1){
  // Insert reflections directly after the skin silhouette, behind straps/spots.
  p.torsoSvg=p.torsoSvg.replace(/^(<path[^>]*\/>)/,`$1<path data-frog-wet-skin="back" d="M 33 50 Q 32 57 34 61 M 36 47 L 38 45" fill="none" stroke="${shine}" stroke-width="1.7" stroke-linecap="round" opacity=".48"/>`);
 }
 if(!back){
  // Remove the original flat throat mark which competed with the mouth.
  if(skin===0)p.torsoSvg=p.torsoSvg.replace(/<path d="M 42 46 C 42 46, 60 48, 78 46 Z"[^>]*\/>/,'');
  if(expression==='normal'&&!stage){
   p.faceSvg=p.faceSvg.replace(/<path d="M 39 46\.5 Q 60 44 81 46\.5"[^>]*\/>\s*<path d="M 39 46\.5 q -2 0\.5 -2\.5 2\.5 M 81 46\.5 q 2 0\.5 2\.5 2\.5"[^>]*\/>/,
    `<g data-frog-mouth="sculpted"><path d="M 39 46.5 Q 59 43.6 81 46.5 Q 64 51.4 44 49.4 Z" fill="${deep}" opacity=".65"/><path d="M 39 46.5 Q 59 43.6 81 46.5" fill="none" stroke="${P.ink}" stroke-width="2.1" stroke-linecap="round"/><path d="M 38.5 46 Q 37 46.8 37.5 48 M 81.5 46 Q 83 46.8 82.5 48" fill="none" stroke="${P.ink}" stroke-width="1.4" stroke-linecap="round"/><path d="M 47 49.6 Q 57 51 66 49.7" fill="none" stroke="${light}" stroke-width="1.2" stroke-linecap="round"/></g>`);
  }
  if(skin===1){
   p.faceSvg=p.faceSvg.replace(/<g transform="rotate\(-12, 78, 48\)">[\s\S]*?<\/g>/,'').replace(/<g fill="none" stroke="#cdd9d4"[\s\S]*?<\/g>/,'');
   if(expression==='normal'){
    const t=Math.max(0,p.time||0);let smoke='';
    for(let i=0;i<3;i++){const f=(t/2.8+i/3)%1,x=100+Math.sin(f*7+i*.4)*f*3,y=43-f*21;smoke+=`<path d="M ${x} ${y+2} q ${2+f*2} -2 ${f*2} -4 q -2 -2 1 -4" fill="none" stroke="#d7e2db" stroke-width="${.65+f*.5}" stroke-linecap="round" opacity="${Math.sin(Math.PI*f)*.65}"/>`;}
    p.faceSvg+=`<g data-sapone-cigar="bitten"><g transform="rotate(-10 79 46)"><path d="M 79 44.8 H 99 Q 101 46.4 99 48 H 79 Z" fill="#78452b" stroke="#101b17" stroke-width="1.1"/><path d="M 83 45.4 H 95" stroke="#ad7850" stroke-width=".7"/><path d="M 89 44.8 V 48" stroke="#dfb85c" stroke-width="2"/><path d="M 99 45 V 47.8" stroke="#ef8142" stroke-width="1.7"/><path d="M 78 44.7 Q 80 45.2 80 46.7" fill="none" stroke="#101b17" stroke-width="1.4" stroke-linecap="round"/></g><g data-sapone-smoke="flow">${smoke}</g></g>`;
   }
  }
 }
 return p;
}


// Helper para montar luva de boxe
function buildGloveSvg(skinId, isRight) {
  if (skinId > 0 && skinId < 4) return refinedFrogGlove(skinId, isRight);
  const flip = isRight ? 'scale(-1, 1)' : '';
  const x = isRight ? 98 : 22;
  const y = 66;

  if (skinId === 0) {
    // Luvas originais do King Frog (Verde floresta com punho escuro e brilho menta)
    return `
    <g transform="translate(${x}, ${y}) scale(0.38) ${flip} translate(-60, -60)">
      <g transform="translate(60, 60) scale(2.5)">
        <rect x="-12" y="-14" width="24" height="28" rx="6" fill="#41643c" stroke="#0f1c10" stroke-width="2"/>
        <rect x="2" y="-4" width="12" height="14" rx="4" fill="#41643c" stroke="#0f1c10" stroke-width="2"/>
        <rect x="-10" y="8" width="20" height="4" fill="#2b4528"/>
        <rect x="-8" y="-10" width="8" height="4" fill="#dcedc1" opacity="0.5"/>
      </g>
    </g>`;
  } else if (skinId === 1) {
    // Dom Sapone: Luvas de couro preto italiano fosco com punho grafite e detalhe vermelho
    return `
    <g transform="translate(${x}, ${y}) scale(0.38) ${flip} translate(-60, -60)">
      <g transform="translate(60, 60) scale(2.5)">
        <rect x="-12" y="-14" width="24" height="28" rx="6" fill="#18181b" stroke="#0f1c10" stroke-width="2"/>
        <rect x="2" y="-4" width="12" height="14" rx="4" fill="#27272a" stroke="#0f1c10" stroke-width="2"/>
        <rect x="-10" y="8" width="20" height="4" fill="#3f3f46"/>
        <rect x="-8" y="-10" width="8" height="3" fill="#71717a" opacity="0.4"/>
        <line x1="-10" y1="7" x2="10" y2="7" stroke="#dc2626" stroke-width="0.8"/>
      </g>
    </g>`;
  } else if (skinId === 2) {
    // Imperador Romano: Luvas Caestus de couro marrom nobre com faixas de amarração romanas cruas e tachas de bronze
    return `
    <g transform="translate(${x}, ${y}) scale(0.38) ${flip} translate(-60, -60)">
      <g transform="translate(60, 60) scale(2.5)">
        <rect x="-12" y="-14" width="24" height="28" rx="6" fill="#78350f" stroke="#0f1c10" stroke-width="2"/>
        <rect x="2" y="-4" width="12" height="14" rx="4" fill="#92400e" stroke="#0f1c10" stroke-width="2"/>
        <!-- Faixas romanas cruzadas no punho -->
        <rect x="-10" y="7" width="20" height="6" fill="#fef3c7" stroke="#0f1c10" stroke-width="1.2"/>
        <line x1="-10" y1="10" x2="10" y2="10" stroke="#b45309" stroke-width="1"/>
        <!-- Placa de bronze nos nós dos dedos -->
        <rect x="-10" y="-12" width="12" height="4" rx="1.5" fill="#eab308" stroke="#0f1c10" stroke-width="1"/>
        <circle cx="-7" cy="-10" r="0.8" fill="#78350f"/>
        <circle cx="-1" cy="-10" r="0.8" fill="#78350f"/>
      </g>
    </g>`;
  } else if (skinId === 3) {
    // Sapo Dardo Xamânico: Luvas amarelo fluorescente vibrante com faixas tribais pretas e mancha tóxica
    return `
    <g transform="translate(${x}, ${y}) scale(0.38) ${flip} translate(-60, -60)">
      <g transform="translate(60, 60) scale(2.5)">
        <rect x="-12" y="-14" width="24" height="28" rx="6" fill="#facc15" stroke="#0f1c10" stroke-width="2"/>
        <rect x="2" y="-4" width="12" height="14" rx="4" fill="#eab308" stroke="#0f1c10" stroke-width="2"/>
        <rect x="-10" y="8" width="20" height="5" fill="#0f172a" stroke="#0f1c10" stroke-width="1"/>
        <!-- Faixa tribal sagrada -->
        <line x1="-10" y1="10" x2="10" y2="10" stroke="#10b981" stroke-width="1.5"/>
        <!-- Mancha preta dardo venenosa -->
        <ellipse cx="-4" cy="-5" rx="4" ry="5" fill="#0f172a"/>
        <rect x="-8" y="-12" width="7" height="3" fill="#ffffff" opacity="0.5"/>
      </g>
    </g>`;
  }
}

// Construtor mestre de SVGs das Skins do Rei Sapo
function renderFrogSvg({ skin = 0, state = 'normal', expression = 'normal', view = 'front', papoStage = 0, poisoned = false, noCrown = false, armed = true, time = 0, partsOnly = false } = {}) {
  const stage = Math.max(0, Math.min(4, Number(state.match(/^papo_([1-4])$/)?.[1] || (state === 'papo_inflado' ? 4 : papoStage)) || 0));
  const poison = poisoned || state === 'pele_venenosa';
  const bare = noCrown || state === 'sem_coroa';
  if (poison) state = 'pele_venenosa';

  // Paletas de cores por Skin
  const palettes = [
    // Skin 0: O Rei da Lama (Padrão Original do Jogo)
    {
      skinTone: state === 'pele_venenosa' ? '#244326' : '#769a55',
      skinDark: state === 'pele_venenosa' ? '#142816' : '#527038',
      bellyTone: state === 'pele_venenosa' ? '#3d6e40' : '#dcedc1',
      bellyDark: state === 'pele_venenosa' ? '#203d22' : '#c2db9f',
      feetTone: state === 'pele_venenosa' ? '#c2410c' : '#d17b24',
      feetDots: state === 'pele_venenosa' ? '#ea580c' : '#f0a050',
      shortsTone: '#2b4528',
      shortsStripe: '#e5cc5b',
      beltTone: '#0f1c10',
      buckleTone: '#e5cc5b',
      ink: '#0f1c10',
      toxicGlow: state === 'pele_venenosa'
    },
    // Skin 1: Dom Sapone, O Mafioso da Lama (The Frogfather)
    {
      skinTone: state === 'pele_venenosa' ? '#27362b' : '#627d59',
      skinDark: state === 'pele_venenosa' ? '#18241b' : '#45593e',
      bellyTone: '#f8fafc', // Camisa social branca linho impecável!
      bellyDark: '#e2e8f0',
      feetTone: '#18181b', // Sapatos sociais de couro preto de mafioso
      feetDots: '#3f3f46',
      shortsTone: '#18181b', // Calça social risca de giz
      shortsStripe: '#334155',
      beltTone: '#09090b',
      buckleTone: '#cbd5e1', // Fivela prateada
      ink: '#0f1c10',
      toxicGlow: state === 'pele_venenosa'
    },
    // Skin 2: Imperador Sapo do Coliseu (Gladiador Romano)
    {
      skinTone: state === 'pele_venenosa' ? '#273824' : '#6b8c4d',
      skinDark: state === 'pele_venenosa' ? '#172415' : '#4c6436',
      bellyTone: '#d97706', // Lorica peitoral de bronze dourado polido!
      bellyDark: '#b45309',
      feetTone: '#78350f', // Sandálias romanas de tiras de couro marrom
      feetDots: '#eab308', // Rebites de ouro
      shortsTone: '#451a03', // Saiote romano de lâminas de couro (pteruges)
      shortsStripe: '#eab308',
      beltTone: '#78350f',
      buckleTone: '#facc15', // Fivela imperial romana
      ink: '#0f1c10',
      toxicGlow: state === 'pele_venenosa'
    },
    // Skin 3: Sapo Dardo Xamânico (Toxic Dartfrog & Cocar Asteca)
    {
      skinTone: state === 'pele_venenosa' ? '#1e3a8a' : '#1d4ed8', // Azul Cobalto Elétrico profundo!
      skinDark: state === 'pele_venenosa' ? '#0f172a' : '#1e40af',
      bellyTone: '#facc15', // Barriga amarelo fluorescente viva!
      bellyDark: '#eab308',
      feetTone: '#2563eb', // Pés azuis com ventosas amarelas
      feetDots: '#facc15',
      shortsTone: '#0f172a', // Calção tribal preto e ouro com runas
      shortsStripe: '#facc15',
      beltTone: '#0284c7',
      buckleTone: '#10b981', // Broche central de pedra jade verde
      ink: '#09090b',
      toxicGlow: state === 'pele_venenosa'
    }
  ];

  const P = palettes[skin] || palettes[0];
  const ink = P.ink;

  // 1. Pernas & Pés
  let legsSvg = '';
  if (skin === 1) {
    // Dom Sapone: Calça social risca de giz com vincos e sapatos de bico fino de mafioso
    legsSvg = `
    <!-- Pernas de Terno Risca de Giz -->
    <path d="M 35 85 V 104 H 49 V 85 Z" fill="#18181b" stroke="${ink}" stroke-width="2"/>
    <path d="M 71 85 V 104 H 85 V 85 Z" fill="#18181b" stroke="${ink}" stroke-width="2"/>
    <!-- Linhas de Risca de Giz nas Pernas -->
    <line x1="42" y1="85" x2="42" y2="104" stroke="#475569" stroke-width="0.9"/>
    <line x1="78" y1="85" x2="78" y2="104" stroke="#475569" stroke-width="0.9"/>
    <!-- Sapatos Sociais de Verniz Bicolor Preto e Branco (Spats) -->
    <path d="M 46 100 Q 30 98 18 103 Q 14 105 16 109 H 48 Z" fill="#18181b" stroke="${ink}" stroke-width="2"/>
    <path d="M 28 100 Q 22 101 19 104 H 36 Q 34 100 28 100 Z" fill="#f8fafc" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="25" cy="102" r="0.8" fill="#18181b"/>
    <circle cx="30" cy="102" r="0.8" fill="#18181b"/>
    
    <g transform="translate(120, 0) scale(-1, 1)">
      <path d="M 46 100 Q 30 98 18 103 Q 14 105 16 109 H 48 Z" fill="#18181b" stroke="${ink}" stroke-width="2"/>
      <path d="M 28 100 Q 22 101 19 104 H 36 Q 34 100 28 100 Z" fill="#f8fafc" stroke="${ink}" stroke-width="1.2"/>
      <circle cx="25" cy="102" r="0.8" fill="#18181b"/>
      <circle cx="30" cy="102" r="0.8" fill="#18181b"/>
    </g>`;
  } else if (skin === 2) {
    // Imperador Romano: Caneleiras de bronze e sandálias romanas com tiras de couro trançadas nas patas
    legsSvg = `
    <!-- Pernas com Caneleiras de Bronze -->
    <path d="M 35 85 V 105 H 48 V 85 Z" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
    <path d="M 72 85 V 105 H 85 V 85 Z" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Caneleira de Bronze Romana (Grevas) -->
    <rect x="36" y="87" width="11" height="15" rx="3" fill="#b45309" stroke="${ink}" stroke-width="1.6"/>
    <rect x="73" y="87" width="11" height="15" rx="3" fill="#b45309" stroke="${ink}" stroke-width="1.6"/>
    <line x1="41.5" y1="88" x2="41.5" y2="101" stroke="#fbbf24" stroke-width="1.2"/>
    <line x1="78.5" y1="88" x2="78.5" y2="101" stroke="#fbbf24" stroke-width="1.2"/>
    <!-- Pés Palmados com Tiras de Couro de Sandália Romana -->
    <path d="M 46 101 Q 33 99 21 103 Q 15 105 16 108.5 Q 21 106.5 24.5 109.5 Q 28.5 106.5 32 109.5 Q 35.5 106.5 39.5 109.5 Q 44 106.5 47.5 108 Z" fill="#78350f" stroke="${ink}" stroke-width="2" stroke-linejoin="round"/>
    <circle cx="16.5" cy="108.2" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="24.5" cy="109.6" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="32" cy="109.6" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="39.5" cy="109.6" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
    <path d="M 22 104 L 38 107 M 24 107 L 36 104" stroke="#fef3c7" stroke-width="1.2"/>
    
    <g transform="translate(120, 0) scale(-1, 1)">
      <path d="M 46 101 Q 33 99 21 103 Q 15 105 16 108.5 Q 21 106.5 24.5 109.5 Q 28.5 106.5 32 109.5 Q 35.5 106.5 39.5 109.5 Q 44 106.5 47.5 108 Z" fill="#78350f" stroke="${ink}" stroke-width="2" stroke-linejoin="round"/>
      <circle cx="16.5" cy="108.2" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
      <circle cx="24.5" cy="109.6" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
      <circle cx="32" cy="109.6" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
      <circle cx="39.5" cy="109.6" r="1.9" fill="#eab308" stroke="${ink}" stroke-width="1.2"/>
      <path d="M 22 104 L 38 107 M 24 107 L 36 104" stroke="#fef3c7" stroke-width="1.2"/>
    </g>`;
  } else {
    // Skin 0 & Skin 3: Patas de sapo autênticas com dedos e membranas
    legsSvg = `
    <!-- Pernas Grossas -->
    <path d="M 35 85 V 105 H 48 V 85 Z" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
    <path d="M 72 85 V 105 H 85 V 85 Z" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Pés com Dedos e Membrana -->
    <path d="M 46 101 Q 33 99 21 103 Q 15 105 16 108.5 Q 21 106.5 24.5 109.5 Q 28.5 106.5 32 109.5 Q 35.5 106.5 39.5 109.5 Q 44 106.5 47.5 108 Z" fill="${P.feetTone}" stroke="${ink}" stroke-width="2" stroke-linejoin="round"/>
    <path d="M 21 105 L 18 107.5 M 27 105 L 24.5 108.5 M 33 105 L 32 108.5 M 39 105 L 39.5 108.5" stroke="${P.skinDark}" stroke-width="1.2" stroke-linecap="round"/>
    <circle cx="16.5" cy="108.2" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="24.5" cy="109.6" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="32" cy="109.6" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="39.5" cy="109.6" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
    
    <g transform="translate(120 0) scale(-1 1)">
      <path d="M 46 101 Q 33 99 21 103 Q 15 105 16 108.5 Q 21 106.5 24.5 109.5 Q 28.5 106.5 32 109.5 Q 35.5 106.5 39.5 109.5 Q 44 106.5 47.5 108 Z" fill="${P.feetTone}" stroke="${ink}" stroke-width="2" stroke-linejoin="round"/>
      <path d="M 21 105 L 18 107.5 M 27 105 L 24.5 108.5 M 33 105 L 32 108.5 M 39 105 L 39.5 108.5" stroke="${P.skinDark}" stroke-width="1.2" stroke-linecap="round"/>
      <circle cx="16.5" cy="108.2" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
      <circle cx="24.5" cy="109.6" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
      <circle cx="32" cy="109.6" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
      <circle cx="39.5" cy="109.6" r="1.9" fill="${P.feetDots}" stroke="${ink}" stroke-width="1.2"/>
    </g>`;
  }

  // 2. Cotocos / Braços de apoio (ficam atrás das luvas)
  const stumpsSvg = `
  <!-- Cotocos dos Braços -->
  <path d="M 30 48 L 22 50 L 24 56 L 30 54 Z" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
  <path d="M 90 48 L 98 50 L 96 56 L 90 54 Z" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>`;

  // 3. Tronco, Roupa e Peito
  let torsoSvg = '';
  if (skin === 1) {
    // Dom Sapone: Terno Risca de Giz Preto Italiano com Camisa Branca, Gravata Borboleta e Cravo Vermelho. SEM PAPO NENHUM!
    torsoSvg = `
    <!-- Corpo Principal / Paletó Preto -->
    <rect x="28" y="35" width="64" height="50" rx="10" fill="#18181b" stroke="${ink}" stroke-width="2"/>
    <!-- Riscas de Giz verticais clássicas no terno -->
    <line x1="34" y1="38" x2="34" y2="72" stroke="#475569" stroke-width="0.9"/>
    <line x1="42" y1="46" x2="42" y2="72" stroke="#475569" stroke-width="0.9"/>
    <line x1="78" y1="46" x2="78" y2="72" stroke="#475569" stroke-width="0.9"/>
    <line x1="86" y1="38" x2="86" y2="72" stroke="#475569" stroke-width="0.9"/>
    <!-- Camisa Social Branca de Linho (Triângulo no Peito) -->
    <polygon points="50,46 70,46 64,68 56,68" fill="#f8fafc" stroke="${ink}" stroke-width="1.6"/>
    <!-- Gravata Borboleta Carmesim de Chefão -->
    <path d="M 54 48 L 60 51 L 54 54 Z M 66 48 L 60 51 L 66 54 Z" fill="#dc2626" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="60" cy="51" r="1.5" fill="#991b1b"/>
    <!-- Lapelas Pontudas do Paletó Cruzando à Frente -->
    <path d="M 38 46 L 50 46 L 57 66 L 46 68 Z" fill="#27272a" stroke="${ink}" stroke-width="1.4"/>
    <path d="M 82 46 L 70 46 L 63 66 L 74 68 Z" fill="#27272a" stroke="${ink}" stroke-width="1.4"/>
    <!-- Botões de Ouro do Paletó -->
    <circle cx="60" cy="62" r="1.4" fill="#fbbf24" stroke="${ink}" stroke-width="0.8"/>
    <!-- Bolso Superior Esquerdo com Cravo Vermelho Vivo -->
    <line x1="72" y1="52" x2="80" y2="52" stroke="${ink}" stroke-width="1.4"/>
    <!-- Cravo Vermelho da Lapela -->
    <circle cx="75" cy="50" r="2.4" fill="#ef4444" stroke="${ink}" stroke-width="1"/>
    <circle cx="75" cy="49" r="1.2" fill="#fca5a5"/>
    <!-- Shorts / Calça Social Preta -->
    <path d="M 28 72 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="#18181b" stroke="${ink}" stroke-width="2"/>
    <!-- Cinto de Couro Preto com Fivela Prateada de Luxo -->
    <rect x="28" y="72" width="64" height="6" fill="#09090b"/>
    <rect x="52" y="70" width="16" height="10" rx="2" fill="#cbd5e1" stroke="${ink}" stroke-width="1.8"/>
    <rect x="56" y="73" width="8" height="4" fill="#09090b"/>`;
  } else if (skin === 2) {
    // Imperador Romano: Lorica de bronze esculpida, toga púrpura e saiote militar (pteruges)
    torsoSvg = `
    <!-- Corpo Principal -->
    <rect x="28" y="35" width="64" height="50" rx="10" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Peitoral Anatômico de Bronze Dourado (Lorica Musculata) -->
    <path d="M 34 46 Q 60 49 86 46 L 84 72 Q 60 74 36 72 Z" fill="#d97706" stroke="${ink}" stroke-width="2"/>
    <!-- Escultura dos Músculos Peitorais e Abdominais Romanos em Bronze -->
    <path d="M 40 52 Q 49 55 58 52 Q 49 50 40 52 Z" fill="#f59e0b" stroke="#78350f" stroke-width="1"/>
    <path d="M 80 52 Q 71 55 62 52 Q 71 50 80 52 Z" fill="#f59e0b" stroke="#78350f" stroke-width="1"/>
    <line x1="60" y1="52" x2="60" y2="70" stroke="#78350f" stroke-width="1.4"/>
    <path d="M 46 62 Q 60 65 74 62" fill="none" stroke="#78350f" stroke-width="1.2"/>
    <!-- Toga Imperial Romana Púrpura no Ombro com Broche Dourado -->
    <path d="M 28 38 L 44 48 L 38 68 L 28 66 Z" fill="#581c87" stroke="${ink}" stroke-width="1.6"/>
    <circle cx="38" cy="44" r="3" fill="#facc15" stroke="${ink}" stroke-width="1.2"/>
    <!-- Saiote Romano de Lâminas de Couro (Pteruges) -->
    <path d="M 28 72 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="#78350f" stroke="${ink}" stroke-width="2"/>
    <!-- Lâminas Pteruges pendentes com tachas de bronze -->
    <rect x="32" y="74" width="8" height="15" rx="1" fill="#92400e" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="36" cy="85" r="1.4" fill="#fbbf24"/>
    <rect x="44" y="74" width="8" height="15" rx="1" fill="#92400e" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="48" cy="85" r="1.4" fill="#fbbf24"/>
    <rect x="68" y="74" width="8" height="15" rx="1" fill="#92400e" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="72" cy="85" r="1.4" fill="#fbbf24"/>
    <rect x="80" y="74" width="8" height="15" rx="1" fill="#92400e" stroke="${ink}" stroke-width="1.2"/>
    <circle cx="84" cy="85" r="1.4" fill="#fbbf24"/>
    <!-- Cinto Militar Romano Dourado -->
    <rect x="28" y="70" width="64" height="6" fill="#b45309" stroke="${ink}" stroke-width="1.5"/>
    <rect x="52" y="68" width="16" height="10" rx="2" fill="#fbbf24" stroke="${ink}" stroke-width="1.8"/>
    <circle cx="60" cy="73" r="2.5" fill="#78350f"/>`;
  } else if (skin === 3) {
    // Sapo Dardo Xamânico: Azul cobalto elétrico com manchas pretas aposemáticas e barriga amarelo vivo
    torsoSvg = `
    <!-- Corpo Principal Azul Cobalto Profundo -->
    <rect x="28" y="35" width="64" height="50" rx="10" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Manchas Aposemáticas Pretas Dendrobates no Dorso -->
    <path d="M 28 40 Q 36 44 32 54 Q 28 50 28 40 Z" fill="#09090b"/>
    <path d="M 92 40 Q 84 44 88 54 Q 92 50 92 40 Z" fill="#09090b"/>
    <!-- Barriga Amarela Fluorescente Sagrada -->
    <path d="M 34 55 Q 60 70 86 55 V 65 Q 60 85 34 65 Z" fill="${P.bellyTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Pinturas Rituais Xamânicas Geométricas Brancas na Barriga -->
    <polyline points="44,59 52,64 60,59 68,64 76,59" fill="none" stroke="#ffffff" stroke-width="1.4" stroke-linecap="round"/>
    <!-- Shorts Tribal com Motivos Astecas -->
    <path d="M 28 72 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="#0f172a" stroke="${ink}" stroke-width="2"/>
    <path d="M 28 84 H 57.9 L 57 88 H 28 Z M 62.1 84 H 92 V 88 H 63 Z" fill="#facc15" stroke="${ink}" stroke-width="1.4"/>
    <rect x="28" y="72" width="64" height="6" fill="#0284c7"/>
    <!-- Amuleto Central de Pedra Jade Verde Sagrada -->
    <ellipse cx="60" cy="74" rx="8" ry="6" fill="#10b981" stroke="${ink}" stroke-width="1.8"/>
    <circle cx="60" cy="74" r="2.5" fill="#34d399"/>`;
  } else {
    // Skin 0: Padrão do Jogo (Corpo Verde, Barrigão Menta, Shorts e Cinturão Dourado)
    torsoSvg = `
    <!-- Corpo Principal -->
    <rect x="28" y="35" width="64" height="50" rx="10" fill="${P.skinTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Barrigão Verde Menta Claro -->
    <path d="M 32 55 Q 60 70 88 55 V 65 Q 60 85 32 65 Z" fill="${P.bellyTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Dobra sutil do papo de descanso -->
    <path d="M 42 46 C 42 46, 60 48, 78 46 Z" fill="${P.bellyTone}" stroke="${ink}" stroke-width="2"/>
    <!-- Shorts Verde Floresta com Friso Dourado -->
    <path d="M 28 72 H 92 V 90 H 63 L 60 84 L 57 90 H 28 Z" fill="${P.shortsTone}" stroke="${ink}" stroke-width="2"/>
    <path d="M 28 84 H 57.9 L 57 88 H 28 Z M 62.1 84 H 92 V 88 H 63 Z" fill="${P.shortsStripe}" stroke="${ink}" stroke-width="2"/>
    <!-- Cinturão de Campeão com Fivela Dourada -->
    <rect x="28" y="72" width="64" height="6" fill="${P.beltTone}"/>
    <rect x="50" y="68" width="20" height="12" rx="2" fill="${P.buckleTone}" stroke="${ink}" stroke-width="2"/>
    <rect x="54" y="72" width="4" height="2" fill="${ink}"/>
    <rect x="62" y="72" width="4" height="2" fill="${ink}"/>
    <rect x="55" y="76" width="10" height="1" fill="${ink}"/>`;
  }

  // 4. Se estiver no estado 'papo_inflado': renderiza o saco vocal gigante inflado (oficial frogFrontSpriteInflated)!
  let inflatedSvg = '';
  if (state === 'papo_inflado') {
    const infFill = skin === 1 ? '#e2e8f0' : skin === 2 ? '#b45309' : skin === 3 ? '#facc15' : P.bellyTone;
    inflatedSvg = `
    <!-- PAPO BALÃO GIGANTE EXPANDIDO PARA FRENTE (Oficial do Jogo) -->
    <ellipse cx="60" cy="58" rx="28" ry="18" fill="${infFill}" stroke="${ink}" stroke-width="2.6"/>
    <ellipse cx="60" cy="59" rx="25" ry="15" fill="${infFill}" opacity="0.9"/>
    <!-- Dobras e Textura de Tensão no Papo -->
    <path d="M 44 54 Q 60 62 76 54" fill="none" stroke="${ink}" stroke-width="1.6" opacity="0.6"/>
    <path d="M 40 60 Q 60 70 80 60" fill="none" stroke="${ink}" stroke-width="1.8" opacity="0.7"/>
    <path d="M 46 66 Q 60 74 74 66" fill="none" stroke="${ink}" stroke-width="1.4" opacity="0.5"/>
    <ellipse cx="50" cy="52" rx="5" ry="2" fill="#ffffff" opacity="0.5"/>`;
  }

  // 5. Cabeça e Traço Superior
  let headDomeSvg = '';
  if (bare) {
    // Cabeça lisa sem chapéu/coroa (oficial frogFrontSpriteNoCrown)
    headDomeSvg = `
    <!-- Cabeça Carequinha de Anfíbio Lisa -->
    <path d="M 34 34 C 34 20, 54 20, 54 32 Q 60 28, 66 32 C 66 20, 86 20, 86 34 V 44 Q 60 50, 34 44 Z" fill="${P.skinTone}" stroke="none"/>
    <path d="M 34 34 C 34 16, 54 16, 54 26 Q 60 22, 66 26 C 66 16, 86 16, 86 34" fill="none" stroke="${ink}" stroke-width="2" stroke-linejoin="round"/>
    <path d="M 52 24 Q 60 22 68 24" fill="none" stroke="${ink}" stroke-width="1.4" opacity="0.6"/>
    <!-- Brilho na careca lisa do sapo -->
    <ellipse cx="60" cy="24" rx="4" ry="1.5" fill="#ffffff" opacity="0.4"/>`;
  } else {
    // Cabeça padrão com recorte sob a coroa/chapéu
    headDomeSvg = `
    <!-- Cabeça com Saliências nos Olhos -->
    <path d="M 34 34 C 34 20, 54 20, 54 32 Q 60 28, 66 32 C 66 20, 86 20, 86 34 V 44 Q 60 50, 34 44 Z" fill="${P.skinTone}" stroke="none"/>
    <path d="M 34 34 C 34 20, 54 20, 54 32 Q 60 28, 66 32 C 66 20, 86 20, 86 34" fill="none" stroke="${ink}" stroke-width="2" stroke-linejoin="round"/>`;
  }

  // Narinas
  const nostrilsSvg = `
  <rect x="52" y="42" width="2" height="2" fill="${ink}"/>
  <rect x="66" y="42" width="2" height="2" fill="${ink}"/>`;

  // 6. Expressões Faciais OFICIAIS DO JOGO
  let faceSvg = '';
  if (expression === 'angry' || (stage > 0 && expression === 'normal')) {
    // Olhos fechados em >< de esforço máximo ou fúria (frogFrontSpriteCharge / frogFrontSpriteInflated)
    faceSvg = `
    <!-- Olhos em >< de Fúria e Concentração Máxima (Oficial do Jogo) -->
    <path d="M 36 32 L 46 36 L 36 40" fill="none" stroke="${ink}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="M 84 32 L 74 36 L 84 40" fill="none" stroke="${ink}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
    <!-- Rugas de Tensão -->
    <line x1="48" y1="33" x2="52" y2="36" stroke="${ink}" stroke-width="1.5"/>
    <line x1="72" y1="33" x2="68" y2="36" stroke="${ink}" stroke-width="1.5"/>
    <!-- Boca Selada Segurando a Carga -->
    <path d="M 39 46.5 Q 60 44 81 46.5" fill="none" stroke="${ink}" stroke-width="2.6" stroke-linecap="round"/>`;
  } else if (expression === 'pain') {
    // Dor do jogo (reactions.js: painEye/painMouth/drops) nas medidas do sapo: olhos em > < apontando
    // para o nariz, boca torta aberta com a fileira de dentes em cima e gotas de suor voando.
    const er = 5.4, ey = 34, mw = 20.4, mx = 60, my = 47, h = mw * 0.55, f = v => v.toFixed(1);
    const eye = (x, d) => `<path d="M ${f(x - er * 0.9 * d)} ${f(ey - er * 0.8)} L ${f(x + er * 0.8 * d)} ${ey} L ${f(x - er * 0.9 * d)} ${f(ey + er * 0.8)}" fill="none" stroke="${ink}" stroke-width="${f(er * 0.45)}" stroke-linecap="round" stroke-linejoin="round"/>`;
    let drops = '';
    for (let i = 0; i < 4; i++) {
      const s = er * 1.3, dx = s * (1.2 + i * 0.9), dy = -s * (0.6 + i * 0.25) + i * i * s * 0.12;
      drops += `<circle cx="${f(81.8 + dx)}" cy="${f(ey + er + dy)}" r="${f(s * (0.32 - i * 0.04))}" fill="${i % 2 ? '#ffffff' : '#9be7ff'}" stroke="${ink}" stroke-width="${f(s * 0.08)}"/>`;
    }
    faceSvg = `
    <g data-game-expression="pain">
    ${eye(42, 1)}${eye(78, -1)}
    <g data-part="pain-grimace">
      <path d="M 40 48.4 Q 60 41.4 80 49.4 L 78.8 53.4 Q 60 46.8 41.2 52.4 Z" fill="#f8f4e6" stroke="${ink}" stroke-width="2.3" stroke-linejoin="round"/>
      <path d="M 40.8 50.4 Q 60 44.2 79.4 51.4" fill="none" stroke="${ink}" stroke-width="1.3"/>
      <path d="M 46 46.6 L 46.3 51.2 M 52 44.9 L 52.2 49.5 M 58 44.1 L 58.1 48.7 M 64 44.3 L 63.9 48.9 M 70 45.6 L 69.8 50.2 M 75.5 47.4 L 75.2 51.8" stroke="${ink}" stroke-width=".9"/>
    </g>
    <g data-part="pain-sweat-drops">${drops}</g>
    </g>`;
  } else if (expression === 'open_mouth') {
    // Bocarra Escancarada Oficial de Ataque da Língua (frogFrontSpriteOpenMouth)
    faceSvg = `
    <!-- Olhos Fechados de Força Máxima >< -->
    <path d="M 35 32 L 46 36 L 35 40" fill="none" stroke="${ink}" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="M 85 32 L 74 36 L 85 40" fill="none" stroke="${ink}" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"/>
    <!-- BOCARRA ESCANCARADA MÁXIMA (CAVIDADE ESCURA LIMPA DO JOGO) -->
    <path d="M 36 44 Q 60 40 84 44 Q 86 58 60 58 Q 34 58 36 44 Z" fill="#18181b" stroke="${ink}" stroke-width="2.4"/>
    <!-- Ponto Rosa de Disparo da Língua no Centro -->
    <circle cx="60" cy="51" r="5" fill="#f43f5e" stroke="${ink}" stroke-width="1.2"/>`;
  } else if (expression === 'mud_spit') {
    // Bocarra Cuspidora de Lama (frogFrontSpriteMudSpit)
    faceSvg = `
    <!-- Olhos Semicerrados em Mira Focada -->
    <path d="M 38 31 L 48 35 V 38 H 38 Z" fill="${ink}"/>
    <rect x="44" y="34" width="2" height="2" fill="white"/>
    <path d="M 82 31 L 72 35 V 38 H 82 Z" fill="${ink}"/>
    <rect x="74" y="34" width="2" height="2" fill="white"/>
    <!-- Bocarra Circular Cuspidora de Lama -->
    <ellipse cx="60" cy="48" rx="10" ry="7" fill="#451a03" stroke="${ink}" stroke-width="2"/>
    <circle cx="60" cy="48" r="4" fill="#1c1917"/>
    <!-- Respingos de Lama nos Cantos dos Lábios -->
    <circle cx="48" cy="51" r="1.8" fill="#78350f"/>
    <circle cx="72" cy="50" r="1.5" fill="#78350f"/>`;
  } else if (expression === 'x' || expression === 'game_over') {
    // Nocaute Oficial do King Frog (frogFrontSpriteGameOver)
    faceSvg = `
    <!-- Olhos em X Clássicos de Nocaute -->
    <path d="M 38 30 L 46 38 M 46 30 L 38 38" fill="none" stroke="${ink}" stroke-width="3.2" stroke-linecap="round"/>
    <path d="M 74 30 L 82 38 M 82 30 L 74 38" fill="none" stroke="${ink}" stroke-width="3.2" stroke-linecap="round"/>
    <!-- Boca Caída Torta de Nocaute -->
    <path d="M 40 48 Q 60 54 80 48" fill="none" stroke="${ink}" stroke-width="2.4" stroke-linecap="round"/>
    <!-- Língua caída de lado -->
    <path d="M 66 49 C 68 54, 76 54, 74 49 Z" fill="#f43f5e" stroke="${ink}" stroke-width="1.2"/>`;
  } else {
    // Normal / Stance Oficial do Jogo
    let cigarSvg = '';
    if (skin === 1) {
      // Dom Sapone tem um charuto cubano aceso no canto da boca!
      cigarSvg = `
      <!-- Charuto Cubano Aceso no Canto da Boca -->
      <g transform="rotate(-12, 78, 48)">
        <rect x="76" y="46" width="16" height="4" rx="1.5" fill="#5c2c16" stroke="${ink}" stroke-width="1"/>
        <rect x="80" y="46" width="3" height="4" fill="#d97706"/> <!-- Anilha de ouro -->
        <rect x="91" y="46" width="2.5" height="4" fill="#ea580c"/> <!-- Brasa acesa -->
        <!-- Cinza e fumaça sutil -->
        <circle cx="94" cy="48" r="1.2" fill="#94a3b8"/>
        <path d="M 95 47 Q 99 44 97 40 Q 95 36 99 33" fill="none" stroke="#e2e8f0" stroke-width="1" opacity="0.6"/>
      </g>`;
    }
    faceSvg = `
    <!-- Olhos Mal Encarados Originais do Jogo -->
    <path d="M 38 30 L 48 34 V 38 H 38 Z" fill="${ink}"/>
    <rect x="43" y="34" width="2" height="2" fill="white"/>
    <path d="M 82 30 L 72 34 V 38 H 82 Z" fill="${ink}"/>
    <rect x="75" y="34" width="2" height="2" fill="white"/>
    <!-- Boca Brava Clássica -->
    <path d="M 39 46.5 Q 60 44 81 46.5" fill="none" stroke="${ink}" stroke-width="2.4" stroke-linecap="round"/>
    <path d="M 39 46.5 q -2 0.5 -2.5 2.5 M 81 46.5 q 2 0.5 2.5 2.5" fill="none" stroke="${ink}" stroke-width="2" stroke-linecap="round"/>
    ${cigarSvg}`;
  }

  // 7. Acessório de Cabeça (Coroa Real, Fedora, Louros, Cocar Asteca) - Só se NÃO estiver 'sem_coroa'!
  let headwearSvg = '';
  if (!bare) {
    if (skin === 0) {
      // Skin 0: Coroa Real de Latão com Veludo Roxo e Rubi (Oficial do Jogo)
      headwearSvg = `
      <!-- Coroa com Tecido Roxo Imperial -->
      <path d="M 42 26 C 45 6, 75 6, 78 26 Z" fill="#6b2c85" stroke="${ink}" stroke-width="2"/>
      <!-- Coroa Dourada de Pontas -->
      <path d="M 42 26 L 38 6 L 50 18 L 60 4 L 70 18 L 82 6 L 78 26 Z" fill="#e5cc5b" stroke="${ink}" stroke-width="2"/>
      <!-- Rubi Lapidado Central -->
      <rect x="58" y="16" width="4" height="4" fill="${ink}"/>
      <rect x="59" y="17" width="2" height="2" fill="#ac3232"/>`;
    } else if (skin === 1) {
      // Skin 1: Chapéu Fedora Gangster Inclinado com Fita Carmesim
      headwearSvg = `
      <!-- Chapéu Fedora Gangster Preto / Grafite -->
      <g transform="translate(60, 24) rotate(-5) translate(-60, -24)">
        <!-- Aba Curva do Fedora -->
        <ellipse cx="60" cy="26" rx="26" ry="6" fill="#18181b" stroke="${ink}" stroke-width="2"/>
        <!-- Copa Dobrada do Chapéu -->
        <path d="M 44 24 C 44 10, 52 11, 56 14 C 60 11, 74 10, 76 24 Z" fill="#27272a" stroke="${ink}" stroke-width="2"/>
        <!-- Fita Carmesim / Bordô de Luxo -->
        <path d="M 44 24 Q 60 27 76 24 L 76 21 Q 60 24 44 21 Z" fill="#991b1b" stroke="${ink}" stroke-width="1.2"/>
        <line x1="56" y1="14" x2="60" y2="22" stroke="${ink}" stroke-width="1.4" opacity="0.6"/> <!-- Dobra central -->
      </g>`;
    } else if (skin === 2) {
      // Skin 2: Coroa de Louros Triunfal Romana de Ouro Maciço
      headwearSvg = `
      <!-- Coroa de Louros Imperial Romana (Ouro com Fitas Púrpuras) -->
      <g>
        <!-- Fitas Púrpuras Caídas Atrás da Cabeça -->
        <path d="M 44 26 Q 38 34 40 42" fill="none" stroke="#581c87" stroke-width="2.6" stroke-linecap="round"/>
        <path d="M 76 26 Q 82 34 80 42" fill="none" stroke="#581c87" stroke-width="2.6" stroke-linecap="round"/>
        <!-- Aro de Ouro da Coroa -->
        <path d="M 38 27 Q 60 21 82 27" fill="none" stroke="#ca8a04" stroke-width="2.6"/>
        <!-- Folhas de Louro Douradas esculpidas com perfeição -->
        <!-- Lado Esquerdo -->
        <ellipse cx="40" cy="24" rx="4.5" ry="2.2" transform="rotate(-30 40 24)" fill="#facc15" stroke="${ink}" stroke-width="1.2"/>
        <ellipse cx="46" cy="21" rx="4.5" ry="2.2" transform="rotate(-20 46 21)" fill="#facc15" stroke="${ink}" stroke-width="1.2"/>
        <ellipse cx="53" cy="19" rx="4.5" ry="2.2" transform="rotate(-10 53 19)" fill="#facc15" stroke="${ink}" stroke-width="1.2"/>
        <!-- Lado Direito -->
        <ellipse cx="80" cy="24" rx="4.5" ry="2.2" transform="rotate(30 80 24)" fill="#facc15" stroke="${ink}" stroke-width="1.2"/>
        <ellipse cx="74" cy="21" rx="4.5" ry="2.2" transform="rotate(20 74 21)" fill="#facc15" stroke="${ink}" stroke-width="1.2"/>
        <ellipse cx="67" cy="19" rx="4.5" ry="2.2" transform="rotate(10 67 19)" fill="#facc15" stroke="${ink}" stroke-width="1.2"/>
        <!-- Medalhão Central de Ouro no Topo da Testa -->
        <circle cx="60" cy="18" r="3" fill="#fbbf24" stroke="${ink}" stroke-width="1.4"/>
        <circle cx="60" cy="18" r="1.2" fill="#b45309"/>
      </g>`;
    } else if (skin === 3) {
      // Skin 3: Cocar Cerimonial Asteca de Plumas de Quetzal com Pedra Jade
      headwearSvg = `
      <!-- Cocar Sagrado Asteca de Quetzal -->
      <g>
        <!-- Plumas Verdes e Azuis de Quetzal se Abrindo em Arco Sagrado -->
        <path d="M 44 26 C 36 12, 34 2, 40 0 C 44 8, 48 18, 48 24 Z" fill="#059669" stroke="${ink}" stroke-width="1.4"/>
        <path d="M 50 24 C 44 10, 44 0, 52 -2 C 55 6, 56 16, 56 22 Z" fill="#0284c7" stroke="${ink}" stroke-width="1.4"/>
        <path d="M 60 22 C 58 8, 58 -4, 60 -6 C 62 -4, 62 8, 60 22 Z" fill="#10b981" stroke="${ink}" stroke-width="1.4"/>
        <path d="M 70 24 C 76 10, 76 0, 68 -2 C 65 6, 64 16, 64 22 Z" fill="#0284c7" stroke="${ink}" stroke-width="1.4"/>
        <path d="M 76 26 C 84 12, 86 2, 80 0 C 76 8, 72 18, 72 24 Z" fill="#059669" stroke="${ink}" stroke-width="1.4"/>
        <!-- Faixa de Cabeça de Ouro com Desenhos Geométricos -->
        <path d="M 42 26 Q 60 22 78 26 L 78 22 Q 60 18 42 22 Z" fill="#facc15" stroke="${ink}" stroke-width="1.6"/>
        <!-- Pedra Jade Central Oval -->
        <ellipse cx="60" cy="21" rx="4" ry="5" fill="#10b981" stroke="${ink}" stroke-width="1.6"/>
        <ellipse cx="60" cy="21" rx="2" ry="3" fill="#6ee7b7"/>
      </g>`;
    }
  }

  // 8. Efeito de Pele Venenosa (Toxinas e Gotas Fluorescentes)
  let toxicSvg = '';
  if (P.toxicGlow) {
    toxicSvg = `
    <!-- Efeito de Toxina Escorrendo / Pústulas Bioluminescentes (Oficial patch5.js) -->
    <ellipse cx="36" cy="56" rx="2.5" ry="3" fill="#4ade80" stroke="#14532d" stroke-width="0.8"/>
    <ellipse cx="84" cy="58" rx="2.5" ry="3" fill="#4ade80" stroke="#14532d" stroke-width="0.8"/>
    <ellipse cx="44" cy="74" rx="2" ry="2.5" fill="#4ade80" stroke="#14532d" stroke-width="0.8"/>
    <ellipse cx="76" cy="74" rx="2" ry="2.5" fill="#4ade80" stroke="#14532d" stroke-width="0.8"/>
    <circle cx="60" cy="36" r="1.8" fill="#4ade80" stroke="#14532d" stroke-width="0.6"/>
    <!-- Vapores Tóxicos -->
    <path d="M 28 50 Q 24 45 28 40" fill="none" stroke="#4ade80" stroke-width="1.2" opacity="0.7"/>
    <path d="M 92 50 Q 96 45 92 40" fill="none" stroke="#4ade80" stroke-width="1.2" opacity="0.7"/>`;
  }

  // 9. LUVAS DE BOXE EM GUARDA! (Left em 22, 66 e Right em 98, 66)
  // Montadas diretamente na frente do corpo, garantindo o pugilista autêntico do Punch-Out!
  const parts = finishFrogParts({ time, skin, stage, poison, bare, view, expression, P, ink, legsSvg, stumpsSvg, torsoSvg, inflatedSvg, headDomeSvg, nostrilsSvg, faceSvg, headwearSvg, toxicSvg });
  const gloveLSvg = skin > 0 ? refinedFrogGlove(skin, false, poison, view) : buildGloveSvg(skin, false);
  const gloveRSvg = skin > 0 ? refinedFrogGlove(skin, true, poison, view) : buildGloveSvg(skin, true);

  if(partsOnly)return {...parts,gloveLSvg,gloveRSvg};
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 120 120" width="100%" height="100%">
    ${parts.style}
    ${parts.background}
    ${parts.legsSvg}
    ${parts.stumpsSvg}
    ${parts.torsoSvg}
    ${parts.inflatedSvg}
    ${parts.headDomeSvg}
    ${parts.nostrilsSvg}
    ${parts.faceSvg}
    ${parts.headwearSvg}
    ${parts.toxicSvg}
    <!-- Luvas de Boxe Oficiais na Guarda -->
    ${armed?gloveLSvg:''}
    ${armed?gloveRSvg:''}
    ${parts.foreground}
  </svg>`;
}



return o=>renderFrogSvg({...o,skin:3,state:o.state==='poison'?'normal':o.state,poisoned:o.state==='poison'||o.poisoned,papoStage:o.papoStage??(o.state==='inflated'?4:0),noCrown:o.state==='nocrown'||o.noCrown});};const drawFrog=makeArt();const skinId=3;
const I=[1,0,0,1,0,0],fmt=x=>Number(x.toFixed(5)),mat=m=>'matrix('+m.map(fmt).join(' ')+')';
function mul(a,b){return[a[0]*b[0]+a[2]*b[1],a[1]*b[0]+a[3]*b[1],a[0]*b[2]+a[2]*b[3],a[1]*b[2]+a[3]*b[3],a[0]*b[4]+a[2]*b[5]+a[4],a[1]*b[4]+a[3]*b[5]+a[5]];}
function inv(m){const d=m[0]*m[3]-m[1]*m[2];return Math.abs(d)<1e-8?I:[m[3]/d,-m[1]/d,-m[2]/d,m[0]/d,(m[2]*m[5]-m[3]*m[4])/d,(m[1]*m[4]-m[0]*m[5])/d];}
function transform(s){let m=I;for(const v of s.matchAll(/(translate|scale|rotate)\(([^)]+)\)/g)){const a=v[2].split(/[,\s]+/).filter(Boolean).map(Number);let n=I;if(v[1]==='translate')n=[1,0,0,1,a[0],a[1]||0];if(v[1]==='scale')n=[a[0],0,0,a[1]??a[0],0,0];if(v[1]==='rotate'){const r=a[0]*Math.PI/180,c=Math.cos(r),q=Math.sin(r),x=a[1]||0,y=a[2]||0;n=[c,q,-q,c,x-c*x+q*y,y-q*x-c*y];}m=mul(m,n);}return m;}
const wrap=(s,m=I,extra='')=>`<g transform="${mat(m)}" ${extra}>${s||''}</g>`;
function detachCrown(svg){const start=svg.indexOf('<g data-frog-crown-seat=');if(start<0)return [svg,''];const parents=[I];for(const v of svg.slice(0,start).matchAll(/<g\b[^>]*>|<\/g>/g)){if(v[0]==='</g>')parents.pop();else parents.push(mul(parents.at(-1),transform(v[0].match(/transform="([^"]+)"/)?.[1]||'')));}const re=/<g\b[^>]*>|<\/g>/g;re.lastIndex=start;let depth=0,m;while((m=re.exec(svg))){depth+=m[0].startsWith('</')?-1:1;if(!depth)return [svg.slice(0,start)+svg.slice(re.lastIndex),wrap(svg.slice(start,re.lastIndex),parents.at(-1))];}return [svg,''];}
function legsWithPose(svg,pose){
 // Split compound rear-leg paths so both feet, seams and highlights stay bound.
 svg=svg.replace(/<path\b[^>]*d="([^"]+)"[^>]*\/>/g,(tag,d)=>{const parts=d.trim().split(/\s+(?=M\s)/);return parts.length>1?parts.map(p=>tag.replace(d,p)).join(''):tag;});
 const stack=[I];return svg.replace(/<g\b[^>]*>|<\/g>|<(?:path|rect|circle|ellipse|line|polygon|polyline)\b[^>]*\/>/g,tag=>{
  if(tag==='</g>'){stack.pop();return tag;}
  const parent=stack.at(-1);if(tag.startsWith('<g')){stack.push(mul(parent,transform(tag.match(/transform="([^"]+)"/)?.[1]||'')));return tag;}
  const x=Number(tag.match(/\b(?:x|x1|cx)="([-\d.]+)"/)?.[1]??tag.match(/(?:d|points)="[M\s]*([-\d.]+)/)?.[1]??60),worldX=parent[0]*x+parent[4],slot=worldX<60?'legL':'legR';return wrap(tag,mul(mul(inv(parent),pose[slot]||I),parent),`data-frog-limb="${slot}"`);
 });
}
function approvedPieces(o,pose={},turn=0){
 const stage=o.specialMouthStage,opening=typeof stage==='number'?[0,.22,.43,.70,1,.65,.25][stage]||0:stage==='mud_spit'?1:0;
 const noHat=stage==='no_crown'||stage==='squash_no_crown';
 const p=drawFrog({...o,noCrown:noHat||o.noCrown,expression:opening?(stage==='mud_spit'?'mud_spit':'open_mouth'):stage==='charge'?'angry':o.expression,papoStage:stage==='inflated'?4:o.papoStage,partsOnly:true});
 if(opening&&o.view!=='back'){
  p.faceSvg=p.faceSvg.replace(/<(?:path|circle|ellipse)\b[^>]*\/>/g,tag=>{
   const y=Number(tag.match(/\bcy="([-\d.]+)"/)?.[1]??tag.match(/\bd="M\s*[-\d.]+\s+([-\d.]+)/)?.[1]??0);
   return y>=40?`<g transform="translate(0 43) scale(1 ${opening}) translate(0 -43)">${tag}</g>`:tag;
  });
  p.faceSvg=`<g data-native-mouth-stage="${stage}" data-mouth-open="${opening}">${p.faceSvg}</g>`;
 }
 const [dome,underCrown]=detachCrown(p.headDomeSvg),headPose=pose.head||pose.torso||I,crown=noHat?'':underCrown||p.headwearSvg;
 let head=dome+p.nostrilsSvg+p.faceSvg;
 if(turn){const sx=1-Math.abs(turn)*.3;head=`<g transform="translate(${60+turn*4} 0) scale(${sx} 1) translate(-60 0)">${head}</g>`;}
 const crownArt=wrap(crown,pose.crown||headPose,`data-angle-owner="head" data-frog-hat="${o.signatureHeld?'held':'seated'}"`);
 let body=p.style+p.background+legsWithPose(p.legsSvg,pose)+wrap(p.stumpsSvg+p.torsoSvg+p.inflatedSvg,pose.torso||I)+(underCrown?crownArt:'')+wrap(head,headPose,'data-angle-owner="head"')+(underCrown?'':crownArt)+wrap(p.toxicSvg+p.foreground,pose.torso||I);
 if(stage==='squash_no_crown')body=wrap(body,[1.18,0,0,.72,-10.8,31.36],'data-native-leap="squash-no-crown"');
 if(stage==='charge')body=wrap(body,[1.025,0,0,.97,-1.5,3.36],'data-native-mouth-stage="charge"');
 return {p,body};
}
function spin(o){
 const t=(o.time||0)%2.4,p=t/2.4,a=p*Math.PI*4+(o.view==='back'?Math.PI:0),sector=Math.floor((a%(2*Math.PI))/(Math.PI/4)+.5)%8,frame=[0,1,2,3,4,3,2,1][sector],view=frame>=3?'back':'front',turn=[0,.6,1,.6,0][frame],pieces=approvedPieces({...o,view,expression:'game_over'}, {},turn),sx=[1,.82,.5,.82,1][frame]*(sector>4?-1:1),flight=o.action==='eject',x=flight?Math.sin(p*Math.PI)*26:0,y=flight?-Math.sin(p*Math.PI)*35:0,roll=flight?p*396:0;
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-45 -65 210 205" data-combat="${o.action}" data-sector="${sector}" data-angle-frame="${frame}"><g transform="translate(${x} ${y}) rotate(${roll} 60 60) translate(60 0) scale(${sx} 1) translate(-60 0)">${pieces.body}${o.armed?pieces.p.gloveLSvg+pieces.p.gloveRSvg:''}</g></svg>`;
}
function signatureGesture(o,phase,pose){
 const ease=x=>{x=Math.max(0,Math.min(1,x));return x*x*(3-2*x);},head=pose.head||pose.torso||I;
 if(skinId===2){
  // The emperor judges the opponent; his tiara never leaves his head.
  const advance=Math.min(1,phase/.22),reach=advance*advance*(1-ease((phase-.88)/.12)),swap=ease((phase-.16)/.035)*(1-ease((phase-.94)/.035)),angle=180*ease((phase-.52)/.2)*(1-ease((phase-.88)/.12)),cx=98-17*reach,cy=66-12*reach,s=1+.16*reach;
  pose.crown=head;
  const right=mul(head,transform(`translate(${cx} ${cy}) rotate(${angle}) scale(${s}) translate(-98 -66)`));
  // Same caestus, plate, cuff and palette as the approved glove; only the thumb opens.
  const art=`<g data-emperor-thumb="up" transform="translate(98 66) scale(.95 .95)" stroke="#101b17" stroke-linejoin="round"><g data-closed-fingers="sideways" transform="rotate(-90)"><rect x="-12" y="-14" width="24" height="28" rx="6" fill="#633c29" stroke-width="2"/><path d="M -10 -11 H 6 L 8 -4 H -10 Z" fill="#b87536" stroke-width="1.3"/><path d="M -8 -10 H 4" stroke="#f5ce84" stroke-width="1.8" stroke-linecap="round"/><circle cx="-7" cy="-6" r="1" fill="#f5ce84" stroke="none"/><circle cx="4" cy="-6" r="1" fill="#f5ce84" stroke="none"/><rect x="-10" y="7" width="20" height="7" rx="1" fill="#e9dbb6" stroke-width="1.2"/><path d="M -10 8 L 9 12 M -9 12 L 9 8" stroke="#a99b7d" stroke-width="1"/>${o.view==='back'?'<path d="M -7 0 L 1 5 M 1 0 L -7 5" stroke="#ae8560" stroke-width="1.4"/>':''}</g><path data-raised-thumb="perpendicular" d="M 2 -8 L 3 -19 Q 3 -23 7 -23 Q 11 -23 11 -19 L 12 2 Q 12 7 7 7 L 2 4" fill="#835237" stroke-width="2" stroke-linecap="round"/></g>`;
  return {held:false,left:head,right,rightArt:art,swap,thumbAngle:angle,camera:o.armed?transform(`translate(-2 ${3*reach}) translate(60 60) scale(${1+.04*reach}) translate(-60 -60) translate(${2*(1-reach)} 0)`):I};
 }
 const reach=ease(phase/.18),leave=ease((phase-.94)/.06),tuck=ease((phase-.27)/.18)*(1-ease((phase-.73)/.18)),lift=ease((phase-.18)/.09)*(1-ease((phase-.84)/.1));
 const stage=o.state==='inflated'?4:Number(o.state?.match(/^papo_([1-4])$/)?.[1]||0),grip=skinId===0?[50,o.view==='back'?32:29]:skinId===1?[38,28]:[40,26];grip[1]-=stage*1.65;
 const held=!!o.armed&&o.state!=='nocrown'&&phase>=.18&&phase<=.94;
 const gesture=held?mul([1,0,0,1,-18*lift+(10-grip[0])*tuck,-7*lift+(78-grip[1])*tuck],transform(`rotate(${-24*tuck} ${grip[0]} ${grip[1]})`)):I;
 pose.crown=mul(head,gesture);
 const amount=o.state==='nocrown'||!o.armed?0:reach*(1-leave),cx=22+(grip[0]-22)*amount,cy=66+(grip[1]+12-66)*amount;
 // Two deliberate chest thumps: approach, contact, recoil. No vibration while
 // reaching for the hat, and no high-frequency oscillation between impacts.
 const keys=[[0,98,66],[.45,98,66],[.52,82,62],[.57,68,62],[.64,82,62],[.71,68,62],[.79,84,64],[.88,98,66],[1,98,66]];
 let j=0;while(j<keys.length-2&&phase>keys[j+1][0])j++;
 const a=keys[j],b=keys[j+1],q=ease((phase-a[0])/(b[0]-a[0]));
 const right=mul(head,[1,0,0,1,a[1]+(b[1]-a[1])*q-98,a[2]+(b[2]-a[2])*q-66]);
 const recoil=[.57,.71].reduce((sum,contact)=>sum+(phase>=contact&&phase<contact+.09?1-ease((phase-contact)/.09):0),0);
 return {held,left:mul(head,mul(gesture,[1,0,0,1,cx-22,cy-66])),right,grip,recoil};
}
function combatFrog(o){
 const data=o.combat,action=o.action,t=Math.max(0,o.time||0),view=o.view==='back'?'B':'F';
 if(action==='spin'||action==='eject')return spin(o);
 if(action==='inflate'){
  const u=(t%3.2)/3.2,stage=Math.min(4,Math.max(0,(1-Math.cos(u*Math.PI*2))*2));
  const svg=combatFrog({...o,action:'idle',state:['poison','nocrown'].includes(o.state)?o.state:'normal',papoStage:stage,noCrown:o.state==='nocrown',preservePoison:o.state==='poison'});return svg.replace('data-combat="idle"',`data-combat="inflate" data-inflation-stage="${fmt(stage)}"`);
 }
 const inflated=o.papoStage>0||o.state==='inflated'||/^papo_/.test(o.state),form=o.state==='poison'?(inflated?'toxicInflated':'toxic'):o.state==='nocrown'?'nocrown':inflated?'inflated':'base',c=data.clips[form+'/'+view+'/'+action]||data.clips['base/'+view+'/'+action];
 if(!c)return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-42 -45 204 183" data-combat="${action}" data-unavailable="true"><text x="60" y="50" text-anchor="middle" fill="#bbc6d5" font-family="sans-serif" font-size="7">Não exportado nesta vista.</text><text x="60" y="63" text-anchor="middle" fill="#bbc6d5" font-family="sans-serif" font-size="6">Troque frente / costas.</text></svg>`;
 const duration=o.actionDuration||c.duration+(c.loop?0:.45),phase=c.loop?(t%c.duration)/c.duration:Math.min(1,(t%duration)/c.duration),index=Math.min(c.frames.length-1,Math.floor(phase*(c.frames.length-1))),f=c.frames[index],pose={};for(const inst of f.instances)if(!pose[inst[8]])pose[inst[8]]=inst.slice(1,7);
 // O rosto do sapo é pintado sobre o tronco (não há pescoço): cabeça presa ao tronco; a coroa mantém só o atraso relativo à cabeça.
 if(pose.torso&&pose.head){if(pose.crown)pose.crown=mul(pose.torso,mul(inv(pose.head),pose.crown));pose.head=pose.torso;}
 // The imported signature contains a rapid pre-contact shake. Use our authored
 // gesture on a grounded body instead; only the two actual thumps cause recoil.
 if(action==='signature')for(const slot of Object.keys(pose))pose[slot]=I;
 const signature=action==='signature'?signatureGesture(o,phase,pose):null;
 const hit=action.startsWith('hit'),expression=hit?'pain':action==='groggy'?'game_over':o.expression||'normal',turn=(action==='hit1'||action==='hit2')?Math.sin(Math.PI*phase)*.75:0;
 const punch=/^(jab|cross|hook|body|upper)_/.test(action),nativeMouth=o.specialMouthStage??(punch&&phase>=.18&&phase<=.65?1:action==='telegraph_L'?'charge':0);
 const pieces=approvedPieces({...o,specialMouthStage:nativeMouth,expression,signatureHeld:signature?.held,poisoned:o.preservePoison||o.state==='poison'},pose,turn);
 const rest=data.trajectories[view+'_idle'][0],T=data.trajectories[view+'_'+action],tr=T?T[Math.min(T.length-1,Math.round(phase*(T.length-1)))]:rest,unit=rest[2]/120,A=tr[2]/rest[2],D=tr[3]/rest[3],C=tr[4]*D,root=signature?[1,0,0,1,0,1.5*(signature.recoil||0)]:[A,0,C,D,60+(tr[0]-rest[0])/unit-60*A-120*C,120+(tr[1]-rest[1])/unit-120*D];
 let back='',front='';
 if(o.armed)for(let i=0;i<2;i++){
  if(action==='signature'){
   const m=i?signature.right:signature.left;
   const art=i&&signature.rightArt?`<g data-signature-glove="normal" opacity="${fmt(1-signature.swap)}">${pieces.p.gloveRSvg}</g><g data-signature-glove="judgement" opacity="${fmt(signature.swap)}">${signature.rightArt}</g>`:i?pieces.p.gloveRSvg:pieces.p.gloveLSvg;
   front+=wrap(wrap(art,m,`data-frog-hand="${i?'right':'left'}"`),root);continue;
  }
  const G=tr[5+i],R=rest[5+i],left=R[0]<rest[6-i][0],cx=left?22:98,cy=66,tx=60+(G[0]-rest[0])/unit,ty=120+(G[1]-G[2]/2-rest[1])/(rest[3]/120),s=G[2]/R[2],hand=`<g data-frog-hand="${left?'left':'right'}" data-hand-center="${fmt(tx)} ${fmt(ty)}" transform="translate(${tx} ${ty}) rotate(${G[3]*180/Math.PI}) scale(${s}) translate(${-cx} ${-cy})">${left?pieces.p.gloveLSvg:pieces.p.gloveRSvg}</g>`;
  if(G[5])front+=hand;else back+=hand;
 }
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${action.startsWith('upper')?'-65 -130 250 270':'-42 -45 204 183'}" data-combat="${action}" data-frame="${index}">${wrap(`<ellipse cx="60" cy="115" rx="38" ry="4" fill="#000" opacity=".18"/>${back}${wrap(pieces.body,root,`opacity="${f.opacity}"`)}${front}`,signature?.camera||I,'data-frog-camera="study"')}</svg>`;
}
return o=>{
 if(o.exportSpecialParts){const a=approvedPieces({...o,specialMouthStage:0},{},o.exportTurn||0),p=a.p,[,under]=detachCrown(p.headDomeSvg);return {body:a.body,hands:[p.gloveLSvg,p.gloveRSvg],hats:p.style+(under||p.headwearSvg)};}
 return o.action&&o.combat?combatFrog(o):drawFrog(o);
};
})();const skillCharacter="frog";const skillSkinId="3";const skillSkinIndex=3;
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
