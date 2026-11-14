// Observatório do Dr. Marcov — bancada 3D em código, separada da arena C++ jogável.
// Piso do lutador em y=0. Medidas herdadas do ringue do jogo via UNIT.
// Materiais-base chapados: luz, retícula e contorno vêm do visualizador.
import { THREE, flat, withOutline } from './core.js';

const UNIT = 0.0122; // mesma escala do ringue jogável e da arena de Pernambuco
const u = n => n * UNIT;
const P = {
  ink:'#10172b', navy:'#152b57', navy2:'#1b3d7a', cobalt:'#2455a8', blue:'#286fc5',
  pale:'#d8e6ee', white:'#eff6f7', gold:'#e2b654', gold2:'#f4ca67', bronze:'#987444',
  cyan:'#59c9dc', teal:'#287b96', red:'#bf4555', deck:'#223752', deck2:'#2b4562',
  glass:'#3a8fac', console:'#244267', screen:'#32c3d2', violet:'#495688'
};
const mats = new Map();
const mat = color => { if (!mats.has(color)) mats.set(color, flat(color)); return mats.get(color); };
const staticGeo = new Map();
function mergeStatic(list){
  const vertices=list.reduce((n,g)=>n+g.getAttribute('position').count,0);
  const indices=list.reduce((n,g)=>n+(g.index?.count||g.getAttribute('position').count),0);
  const positions=new Float32Array(vertices*3),normals=new Float32Array(vertices*3),faces=new Uint32Array(indices);
  let v=0,f=0;
  for(const g of list){
    const p=g.getAttribute('position'),n=g.getAttribute('normal'),ix=g.index;
    positions.set(p.array,v*3);normals.set(n.array,v*3);
    const count=ix?.count||p.count;
    for(let i=0;i<count;i++)faces[f+i]=v+(ix?ix.getX(i):i);
    v+=p.count;f+=count;
  }
  const merged=new THREE.BufferGeometry();
  merged.setAttribute('position',new THREE.BufferAttribute(positions,3));
  merged.setAttribute('normal',new THREE.BufferAttribute(normals,3));
  merged.setIndex(new THREE.BufferAttribute(faces,1));
  return merged;
}
const add = (geometry, color, position=[0,0,0], rotation=[0,0,0]) => {
  const mesh = new THREE.Mesh(geometry, mat(color));
  mesh.position.set(...position); mesh.rotation.set(...rotation); mesh.updateMatrix();
  geometry.applyMatrix4(mesh.matrix);
  if (!staticGeo.has(color)) staticGeo.set(color, []);
  staticGeo.get(color).push(geometry);
};
const box = (w,h,d,x,y,z,color,rotation=0) => add(new THREE.BoxGeometry(u(w),u(h),u(d)),color,[u(x),u(y),u(z)],[0,rotation,0]);
const cyl = (r,h,x,y,z,color,seg=10) => add(new THREE.CylinderGeometry(u(r),u(r),u(h),seg),color,[u(x),u(y),u(z)]);
const line = (a,b,r,color,seg=8) => {
  const A=new THREE.Vector3(...a.map(u)),B=new THREE.Vector3(...b.map(u)),D=B.clone().sub(A);
  const g=new THREE.CylinderGeometry(u(r),u(r),D.length(),seg);
  g.applyMatrix4(new THREE.Matrix4().makeRotationFromQuaternion(new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0,1,0),D.normalize())));
  g.translate((A.x+B.x)/2,(A.y+B.y)/2,(A.z+B.z)/2);
  if(!staticGeo.has(color)) staticGeo.set(color,[]); staticGeo.get(color).push(g);
};
const torus = (r,t,x,y,z,color,rx=Math.PI/2,ry=0) => add(new THREE.TorusGeometry(u(r),u(t),6,48),color,[u(x),u(y),u(z)],[rx,ry,0]);
const pos = (r,a,y=0) => [Math.sin(a)*r,y,Math.cos(a)*r];

function floorTexture(){
  const N=1024,c=document.createElement('canvas'); c.width=c.height=N;
  const g=c.getContext('2d'),k=N/600,X=x=>N/2+x*k,Z=z=>N/2+z*k;
  g.fillStyle=P.navy2;g.fillRect(0,0,N,N);
  g.fillStyle=P.cobalt;g.fillRect(X(-270),Z(-270),540*k,540*k);
  g.strokeStyle=P.gold;g.lineWidth=7*k;g.strokeRect(X(-258),Z(-258),516*k,516*k);
  g.strokeStyle=P.cyan;g.lineWidth=2.5*k;
  for(const r of [105,155]){g.beginPath();g.arc(N/2,N/2,r*k,0,Math.PI*2);g.stroke();}
  for(let i=0;i<8;i++){
    const a=i*Math.PI/4,tip=i%2?91:137,side=i%2?28:35;
    const px=(r,t)=>X(Math.cos(t)*r),py=(r,t)=>Z(Math.sin(t)*r);
    g.fillStyle=i%2?P.pale:P.gold2;g.beginPath();g.moveTo(N/2,N/2);
    g.lineTo(px(side,a-.19),py(side,a-.19));g.lineTo(px(tip,a),py(tip,a));
    g.lineTo(px(side,a+.19),py(side,a+.19));g.closePath();g.fill();
  }
  g.fillStyle=P.navy;g.beginPath();g.arc(N/2,N/2,12*k,0,Math.PI*2);g.fill();
  for(const sx of [-1,1])for(const sz of [-1,1]){
    const x=X(sx*205),y=Z(sz*205);g.strokeStyle=P.gold;g.lineWidth=2*k;
    g.beginPath();g.arc(x,y,21*k,0,Math.PI*2);g.stroke();
    for(let j=0;j<8;j++){const a=j*Math.PI/4;g.beginPath();g.moveTo(x+Math.cos(a)*5*k,y+Math.sin(a)*5*k);g.lineTo(x+Math.cos(a)*17*k,y+Math.sin(a)*17*k);g.stroke();}
  }
  const t=new THREE.CanvasTexture(c);t.colorSpace=THREE.SRGBColorSpace;t.anisotropy=8;return t;
}

function ring(){
  box(600,35,600,0,-21,0,P.ink);
  box(594,7,594,0,-3.5,0,P.gold);
  box(580,5,580,0,-0.4,0,P.navy);
  const floor=new THREE.Mesh(new THREE.PlaneGeometry(u(540),u(540)),new THREE.MeshToonMaterial({map:floorTexture(),gradientMap:mat(P.white).gradientMap}));
  floor.rotation.x=-Math.PI/2;floor.position.y=u(2.5);floor.name='Rosa dos ventos';
  // será acrescentado fora do lote geométrico, mantendo um único material/texture para o tampo.
  const corners=[[-270,-270],[270,-270],[270,270],[-270,270]];
  for(let i=0;i<4;i++){
    const [x,z]=corners[i],color=i<2?P.blue:P.red;
    box(26,128,26,x,61,z,P.navy);
    box(30,16,30,x,125,z,P.gold);
    for(const h of [38,68,98])box(22,20,22,x,h,z,i%2?P.pale:color);
    box(38,14,38,x,-27,z,P.gold);
  }
  for(const h of [38,68,98])for(let side=0;side<4;side++){
    const [ax,az]=corners[side],[bx,bz]=corners[(side+1)%4];
    if(window.__ropeInfo){window.__ropeInfo.push({y:u(h),color:h===68?P.pale:P.white});continue;}   // o jogo 3D desenha as cordas (tools3d/export)
    line([ax,h,az],[bx,h,bz],2.4,h===68?P.pale:side===0?P.blue:side===2?P.red:P.white,10);
  }
  return floor;
}

function deck(){
  // Anel de observação em torno do ringue. Os setores partilham cor e são mesclados no fim.
  const sectors=32,inner=260,outer=1360;
  for(let i=0;i<sectors;i++){
    const a=i*Math.PI*2/sectors,b=(i+1)*Math.PI*2/sectors;
    const vertices=[...pos(inner,a,-38),...pos(outer,a,-38),...pos(outer,b,-38),...pos(inner,b,-38)].map(u);
    const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.Float32BufferAttribute(vertices,3));g.setIndex([0,1,2,0,2,3]);g.computeVertexNormals();
    if(!staticGeo.has(i%2?P.deck:P.deck2))staticGeo.set(i%2?P.deck:P.deck2,[]);
    staticGeo.get(i%2?P.deck:P.deck2).push(g);
    const p=pos(1290,(a+b)/2,-37);cyl(7,2,...p,P.cyan,6);
  }
  torus(inner,6,0,-37,0,P.bronze);torus(outer,9,0,-36,0,P.gold);
  for(let i=0;i<16;i++){
    const a=i*Math.PI*2/16,b=(i+1)*Math.PI*2/16;
    line(pos(1330,a,54),pos(1330,b,54),2.2,P.gold,6);
    if(i%2===0)line(pos(1330,a,-35),pos(1330,a,54),3.8,P.navy,6);
  }
}

function dome(){
  // Colunas e arcos deixam o teto parcialmente aberto: o céu e os astros continuam mundo 3D.
  const R=1250,n=12;
  for(let i=0;i<n;i++){
    const a=i*Math.PI*2/n;
    line(pos(R,a,-38),pos(R,a,450),18,P.navy,8);
    cyl(27,20,...pos(R,a,441),P.gold,8);
    const foot=pos(R,a,-20),top=pos(R,a,452);
    line([foot[0]*.99,foot[1],foot[2]*.99],[top[0]*.99,top[1],top[2]*.99],3,P.gold,6);
    for(let j=0;j<5;j++){
      const t=j/5,tt=(j+1)/5;
      const r=v=>R*(1-v*.68),y=v=>450+Math.sin(v*Math.PI*.85)*400;
      line(pos(r(t),a,y(t)),pos(r(tt),a,y(tt)),13,P.navy2,8);
      line(pos(r(t)-7,a,y(t)+4),pos(r(tt)-7,a,y(tt)+4),2.5,P.bronze,6);
    }
  }
  for(const yy of [365,452])for(let i=0;i<24;i++){
    const a=i*Math.PI*2/24,b=(i+1)*Math.PI*2/24;
    line(pos(R,a,yy),pos(R,b,yy),5,yy===452?P.gold:P.pale,6);
  }
}

function telescope(){
  // Principal atrás do ringue, ligeiramente à direita: lente não invade a linha do oponente.
  const x=380,z=-820;
  cyl(90,20,x,45,z,P.navy,12);cyl(52,140,x,123,z,P.navy2,10);
  cyl(66,16,x,198,z,P.gold,12);
  const a=[x+75,215,z+55],b=[x-130,475,z-90];
  line(a,b,40,P.navy,14);line(a,b,29,P.cobalt,14);
  const lerp=t=>a.map((v,i)=>v+(b[i]-v)*t);
  for(const t of [.13,.58,.91])line(lerp(t-.025),lerp(t+.025),45,P.gold,14);
  line(lerp(.96),b,49,P.navy2,14);
  line(b,[b[0]-3,b[1]+4,b[2]-2],34,P.cyan,14);
  line([x,195,z],[x+110,105,z+65],7,P.bronze,8);
  cyl(24,16,x+112,105,z+65,P.gold,10);
}

function armillary(){
  const x=-690,z=-720,y=255;
  cyl(69,20,x,37,z,P.navy,12);cyl(21,170,x,128,z,P.bronze,10);
  for(const rot of [[Math.PI/2,0,0],[0,Math.PI/2,0],[Math.PI/3,Math.PI/4,0]])
    add(new THREE.TorusGeometry(u(102),u(4),6,40),P.gold,[u(x),u(y),u(z)],rot);
  add(new THREE.IcosahedronGeometry(u(28),1),P.gold2,[u(x),u(y),u(z)]);
}

function consoleAt(x,z,rot){
  box(98,57,54,x,14,z,P.navy,rot);
  box(85,9,47,x,48,z,P.gold,rot);
  box(65,37,5,x,72,z-19,P.console,rot);
  box(50,23,6,x,73,z-23,P.screen,rot);
  for(let i=0;i<4;i++)box(8,3,5,x-23+i*15,54,z+11,i%2?P.gold:P.cyan,rot);
}

function consoles(){
  [[-940,-450,.24],[-1040,250,-.2],[920,-440,-.24],[1020,240,.2],[-450,-1110,.1],[450,-1110,-.1]].forEach(v=>consoleAt(...v));
}

function planets(root){
  const spec=[[-220,540,-940,35,P.gold2],[130,610,-1090,23,P.cyan],[730,410,-650,26,P.red],[-850,450,120,32,P.blue]];
  for(let i=0;i<spec.length;i++){
    const [x,y,z,r,color]=spec[i],mesh=new THREE.Mesh(new THREE.IcosahedronGeometry(u(r),1),mat(color));
    mesh.position.set(u(x),u(y),u(z));mesh.userData.anim={k:'bob',a:u(6)};
    const y0=mesh.position.y,phase=i*1.6;
    mesh.onBeforeRender=()=>{const t=performance.now()*.00025;mesh.position.y=y0+u(6)*Math.sin(t+phase);mesh.rotation.y=t*.3;};
    root.add(mesh);
    line([x,y+60,z],[x,y+r,z],1.8,P.bronze,5);
    if(i===0||i===3)torus(r*1.6,2,x,y,z,P.gold,Math.PI/2,.3);
  }
}

function starfield(root){
  const positions=[];
  for(let i=0;i<130;i++){
    const a=i*2.3999632,r=850+((i*37)%9)*95;
    const y=330+((i*71)%400);
    positions.push(u(Math.sin(a)*r),u(y),u(Math.cos(a)*r));
  }
  const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.Float32BufferAttribute(positions,3));
  const m=new THREE.PointsMaterial({color:P.gold2,size:u(5),sizeAttenuation:true,depthWrite:false});
  root.add(new THREE.Points(g,m));
}

export function buildMarcovArena(){
  staticGeo.clear();
  const root=new THREE.Group();root.name='Observatório do Dr. Marcov';
  const floor=ring();deck();dome();telescope();armillary();consoles();planets(root);
  for(const [color,list] of staticGeo){
    const merged=mergeStatic(list);
    const mesh=new THREE.Mesh(merged,mat(color));mesh.name='Estrutura '+color;
    root.add(withOutline(mesh,.008));
    for(const g of list)g.dispose();
  }
  root.add(floor);starfield(root);
  root.userData.look={tint:[.75,.86,1.12],horizon:'#142b50',zenith:'#071229',fog:[65,175],
    sun:{color:'#bddaf5',int:1.25,pos:[-2,4,-3]},hemi:{sky:'#94b8e8',ground:'#14253c',int:1.25}};
  return root;
}
