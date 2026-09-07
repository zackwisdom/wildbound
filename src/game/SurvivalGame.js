import * as THREE from 'three';

const rand = (a, b) => a + Math.random() * (b - a);
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));

function smoothNoise(seed) {
  const size = 16;
  const grid = [];
  for (let i = 0; i <= size; i++) {
    grid[i] = [];
    for (let j = 0; j <= size; j++) grid[i][j] = Math.random();
  }
  return (x, z) => {
    const scale = 0.04;
    const fx = (x * scale + seed) % size;
    const fz = (z * scale + seed * 1.7) % size;
    const x0 = Math.floor(fx), z0 = Math.floor(fz);
    const x1 = (x0 + 1) % size, z1 = (z0 + 1) % size;
    const tx = fx - x0, tz = fz - z0;
    const a = grid[x0][z0], b = grid[x1][z0], c = grid[x0][z1], d = grid[x1][z1];
    return THREE.MathUtils.lerp(
      THREE.MathUtils.lerp(a, b, tx),
      THREE.MathUtils.lerp(c, d, tx),
      tz
    );
  };
}

export class SurvivalGame {
  constructor(container, callbacks = {}) {
    this.container = container;
    this.callbacks = callbacks;
    this.disposed = false;

    this.health = 100;
    this.maxHealth = 100;
    this.hunger = 100;
    this.stamina = 100;
    this.alive = true;
    this.day = 1;
    this.timeOfDay = 0.25;

    this.inventory = { wood: 0, stone: 0, food: 0 };
    this.selectedSlot = 0;
    this.unlocked = { axe: false, pickaxe: false };

    this.collidables = [];
    this.enemies = [];
    this.placedBlocks = [];
    this.projectiles = [];

    this.keys = {};
    this.mouseDown = false;
    this.lastAttack = 0;

    this._initThree();
    this._buildWorld();
    this._initControls();
    this._animate = this._animate.bind(this);
    this.clock = new THREE.Clock();
    this.renderer.setAnimationLoop(this._animate);
    this._emitState();
  }

  _initThree() {
    const w = this.container.clientWidth;
    const h = this.container.clientHeight;

    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x71879a);
    this.scene.fog = new THREE.Fog(0x71879a, 32, 118);

    this.camera = new THREE.PerspectiveCamera(75, w / h, 0.1, 500);
    this.camera.position.set(0, 12, 0);
    this.scene.add(this.camera);

    this.renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: 'high-performance' });
    this.renderer.setSize(w, h);
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 0.92;
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.container.appendChild(this.renderer.domElement);

    this.ambient = new THREE.AmbientLight(0xa9bfd0, 0.28);
    this.scene.add(this.ambient);
    this.hemisphere = new THREE.HemisphereLight(0x9ec4df, 0x2f3528, 0.5);
    this.scene.add(this.hemisphere);

    this.sun = new THREE.DirectionalLight(0xffe3b0, 1.35);
    this.sun.position.set(50, 80, 30);
    this.sun.castShadow = true;
    this.sun.shadow.mapSize.set(2048, 2048);
    this.sun.shadow.camera.left = -60;
    this.sun.shadow.camera.right = 60;
    this.sun.shadow.camera.top = 60;
    this.sun.shadow.camera.bottom = -60;
    this.sun.shadow.camera.near = 1;
    this.sun.shadow.camera.far = 200;
    this.sun.shadow.bias = -0.00025;
    this.sun.shadow.normalBias = 0.03;
    this.scene.add(this.sun);
    this.scene.add(this.sun.target);

    this.playerLight = new THREE.PointLight(0xffcc88, 0, 18, 2);
    this.scene.add(this.playerLight);

    this._buildPlayerModel();

    this._onResize = () => {
      const ww = this.container.clientWidth;
      const hh = this.container.clientHeight;
      this.camera.aspect = ww / hh;
      this.camera.updateProjectionMatrix();
      this.renderer.setSize(ww, hh);
    };
    window.addEventListener('resize', this._onResize);
  }

  _buildPlayerModel() {
    const skinMat = new THREE.MeshStandardMaterial({ color: 0xc58f68, roughness: 0.92, metalness: 0 });
    const sleeveMat = new THREE.MeshStandardMaterial({ color: 0x303a31, roughness: 0.98, metalness: 0 });
    const shirtMat = new THREE.MeshStandardMaterial({ color: 0x252c27, roughness: 1, metalness: 0 });
    const pantsMat = new THREE.MeshStandardMaterial({ color: 0x242827, roughness: 1, metalness: 0 });
    const bootMat = new THREE.MeshStandardMaterial({ color: 0x171a18, roughness: 0.88, metalness: 0.02 });

    this.playerBody = new THREE.Group();

    const torso = new THREE.Mesh(new THREE.BoxGeometry(0.72, 0.72, 0.34), shirtMat);
    torso.position.y = 1.16;
    torso.castShadow = true;
    torso.receiveShadow = true;
    this.playerBody.add(torso);

    const belt = new THREE.Mesh(new THREE.BoxGeometry(0.75, 0.11, 0.37), bootMat);
    belt.position.y = 0.79;
    belt.castShadow = true;
    this.playerBody.add(belt);

    const legGeo = new THREE.CapsuleGeometry(0.17, 0.58, 5, 8);
    const leftLeg = new THREE.Mesh(legGeo, pantsMat);
    leftLeg.position.set(-0.2, 0.47, 0);
    leftLeg.castShadow = true;
    const rightLeg = leftLeg.clone();
    rightLeg.position.x = 0.2;
    this.playerBody.add(leftLeg, rightLeg);

    const bootGeo = new THREE.BoxGeometry(0.28, 0.2, 0.48);
    const leftBoot = new THREE.Mesh(bootGeo, bootMat);
    leftBoot.position.set(-0.2, 0.12, -0.07);
    leftBoot.castShadow = true;
    const rightBoot = leftBoot.clone();
    rightBoot.position.x = 0.2;
    this.playerBody.add(leftBoot, rightBoot);

    this.playerBody.userData.leftLeg = leftLeg;
    this.playerBody.userData.rightLeg = rightLeg;
    this.scene.add(this.playerBody);

    this.firstPersonArms = new THREE.Group();
    this.camera.add(this.firstPersonArms);

    const makeArm = (side) => {
      const arm = new THREE.Group();
      const x = side * 0.43;
      const sleeve = new THREE.Mesh(new THREE.CapsuleGeometry(0.105, 0.43, 5, 8), sleeveMat);
      sleeve.rotation.x = -0.98;
      sleeve.position.set(x, -0.43, -0.63);
      arm.add(sleeve);

      const wrist = new THREE.Mesh(new THREE.CapsuleGeometry(0.092, 0.17, 5, 8), skinMat);
      wrist.rotation.x = -0.98;
      wrist.position.set(x, -0.55, -0.88);
      arm.add(wrist);

      const hand = new THREE.Mesh(new THREE.BoxGeometry(0.19, 0.16, 0.25), skinMat);
      hand.position.set(x, -0.61, -1.05);
      hand.rotation.x = -0.08;
      hand.rotation.z = side * 0.06;
      arm.add(hand);
      return arm;
    };

    this.leftFirstPersonArm = makeArm(-1);
    this.rightFirstPersonArm = makeArm(1);
    this.firstPersonArms.add(this.leftFirstPersonArm, this.rightFirstPersonArm);
    this.playerWalkPhase = 0;
  }

  _updatePlayerModel(dt, moving, sprinting) {
    if (!this.playerBody || !this.firstPersonArms) return;
    this.playerBody.position.set(this.camera.position.x, this.camera.position.y - 1.7, this.camera.position.z);
    this.playerBody.rotation.y = this.yaw;

    const animSpeed = sprinting ? 12 : 8;
    if (moving && this.onGround) this.playerWalkPhase += dt * animSpeed;
    const walk = moving && this.onGround ? Math.sin(this.playerWalkPhase) : 0;
    const counterWalk = moving && this.onGround ? Math.sin(this.playerWalkPhase + Math.PI) : 0;
    const intensity = sprinting ? 1.25 : 0.75;

    const leftLeg = this.playerBody.userData.leftLeg;
    const rightLeg = this.playerBody.userData.rightLeg;
    if (leftLeg && rightLeg) {
      leftLeg.rotation.x = walk * 0.34 * intensity;
      rightLeg.rotation.x = counterWalk * 0.34 * intensity;
    }

    const idle = Math.sin(performance.now() * 0.0018) * 0.006;
    const bobY = moving && this.onGround ? Math.abs(walk) * 0.018 * intensity : 0;
    const bobX = moving && this.onGround ? Math.sin(this.playerWalkPhase * 0.5) * 0.012 * intensity : 0;
    this.firstPersonArms.position.set(bobX, idle - bobY, 0);
    this.firstPersonArms.rotation.z = moving && this.onGround ? walk * 0.012 * intensity : 0;
    this.leftFirstPersonArm.rotation.x = moving && this.onGround ? counterWalk * 0.035 * intensity : 0;
    this.rightFirstPersonArm.rotation.x = moving && this.onGround ? walk * 0.035 * intensity : 0;
  }

  _heightAt(x, z) {
    const n1 = this._noise(x, z);
    const n2 = this._noise(x * 0.5 + 100, z * 0.5 + 100);
    return (n1 * 0.7 + n2 * 0.3) * 6 - 2;
  }

  _buildWorld() {
    this._noise = smoothNoise(42);
    const size = 200;
    const segs = 120;
    const geo = new THREE.PlaneGeometry(size, size, segs, segs);
    geo.rotateX(-Math.PI / 2);
    const pos = geo.attributes.position;
    const colors = [];
    const colorGrass = new THREE.Color(0x3f6b38);
    const colorMoss = new THREE.Color(0x294b2b);
    const colorDirt = new THREE.Color(0x5b4932);
    const colorSand = new THREE.Color(0x9d9272);
    const colorStone = new THREE.Color(0x5f625c);
    for (let i = 0; i < pos.count; i++) {
      const x = pos.getX(i);
      const z = pos.getZ(i);
      const y = this._heightAt(x, z);
      pos.setY(i, y);
      const localNoise = this._noise(x * 2.7 + 31, z * 2.7 - 19);
      const c = colorGrass.clone().lerp(colorMoss, localNoise * 0.38);
      if (y < -0.65) c.lerp(colorSand, 0.72);
      else if (y > 3.1) c.lerp(colorDirt, 0.48);
      if (y > 4.1) c.lerp(colorStone, clamp((y - 4.1) / 2.5, 0, 0.72));
      c.multiplyScalar(0.9 + localNoise * 0.18);
      colors.push(c.r, c.g, c.b);
    }
    geo.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
    geo.computeVertexNormals();
    const mat = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 1, metalness: 0 });
    this.ground = new THREE.Mesh(geo, mat);
    this.ground.receiveShadow = true;
    this.scene.add(this.ground);

    this._treeGroup = new THREE.Group();
    this.scene.add(this._treeGroup);
    for (let i = 0; i < 70; i++) {
      const x = rand(-90, 90), z = rand(-90, 90);
      if (Math.hypot(x, z) < 8) continue;
      this._spawnTree(x, z);
    }
    for (let i = 0; i < 40; i++) {
      const x = rand(-90, 90), z = rand(-90, 90);
      if (Math.hypot(x, z) < 8) continue;
      this._spawnRock(x, z);
    }
    for (let i = 0; i < 30; i++) {
      const x = rand(-90, 90), z = rand(-90, 90);
      if (Math.hypot(x, z) < 6) continue;
      this._spawnBush(x, z);
    }
    this._spawnGroundDetails();

    const skyGeo = new THREE.SphereGeometry(250, 32, 16);
    const skyMat = new THREE.ShaderMaterial({
      side: THREE.BackSide,
      uniforms: {
        topColor: { value: new THREE.Color(0x4a90c2) },
        bottomColor: { value: new THREE.Color(0xcfe8ff) },
        offset: { value: 33 },
        exponent: { value: 0.6 },
      },
      vertexShader: `varying vec3 vWorldPosition; void main(){ vec4 worldPosition=modelMatrix*vec4(position,1.0); vWorldPosition=worldPosition.xyz; gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.0); }`,
      fragmentShader: `uniform vec3 topColor; uniform vec3 bottomColor; uniform float offset; uniform float exponent; varying vec3 vWorldPosition; void main(){ float h=normalize(vWorldPosition+offset).y; gl_FragColor=vec4(mix(bottomColor,topColor,max(pow(max(h,0.0),exponent),0.0)),1.0); }`,
    });
    this.sky = new THREE.Mesh(skyGeo, skyMat);
    this.scene.add(this.sky);

    const starPositions = [];
    for (let i = 0; i < 650; i++) {
      const v = new THREE.Vector3(rand(-1, 1), rand(0.08, 1), rand(-1, 1)).normalize().multiplyScalar(rand(165, 225));
      starPositions.push(v.x, v.y, v.z);
    }
    const starGeo = new THREE.BufferGeometry();
    starGeo.setAttribute('position', new THREE.Float32BufferAttribute(starPositions, 3));
    this.starMaterial = new THREE.PointsMaterial({ color: 0xdce9ff, size: 0.65, transparent: true, opacity: 0, depthWrite: false, fog: false, sizeAttenuation: true });
    this.stars = new THREE.Points(starGeo, this.starMaterial);
    this.scene.add(this.stars);

    this.moonMaterial = new THREE.MeshBasicMaterial({ color: 0xdde7ef, transparent: true, opacity: 0, fog: false });
    this.moon = new THREE.Mesh(new THREE.SphereGeometry(2.8, 18, 12), this.moonMaterial);
    this.scene.add(this.moon);
  }

  _spawnGroundDetails() {
    const dummy = new THREE.Object3D();
    const grassGeo = new THREE.ConeGeometry(0.11, 0.56, 3);
    grassGeo.translate(0, 0.28, 0);
    const grass = new THREE.InstancedMesh(grassGeo, new THREE.MeshStandardMaterial({ color: 0x355a31, roughness: 1, flatShading: true }), 320);
    for (let i = 0; i < 320; i++) {
      const x = rand(-97, 97), z = rand(-97, 97), y = this._heightAt(x, z), scale = rand(0.6, 1.55);
      dummy.position.set(x, y + 0.02, z); dummy.rotation.set(0, rand(0, Math.PI * 2), 0); dummy.scale.set(scale, scale, scale); dummy.updateMatrix(); grass.setMatrixAt(i, dummy.matrix);
    }
    grass.instanceMatrix.needsUpdate = true;
    this.scene.add(grass);

    const pebbles = new THREE.InstancedMesh(new THREE.DodecahedronGeometry(0.16, 0), new THREE.MeshStandardMaterial({ color: 0x70726d, roughness: 1, flatShading: true }), 90);
    for (let i = 0; i < 90; i++) {
      const x = rand(-96, 96), z = rand(-96, 96), y = this._heightAt(x, z);
      dummy.position.set(x, y + 0.08, z); dummy.rotation.set(rand(0, 1), rand(0, Math.PI * 2), rand(0, 1)); dummy.scale.set(rand(0.45, 1.2), rand(0.35, 0.9), rand(0.55, 1.35)); dummy.updateMatrix(); pebbles.setMatrixAt(i, dummy.matrix);
    }
    pebbles.instanceMatrix.needsUpdate = true;
    this.scene.add(pebbles);

    const branchGeo = new THREE.CylinderGeometry(0.035, 0.055, 1.1, 5);
    branchGeo.rotateZ(Math.PI / 2);
    const branches = new THREE.InstancedMesh(branchGeo, new THREE.MeshStandardMaterial({ color: 0x4a3525, roughness: 1 }), 46);
    for (let i = 0; i < 46; i++) {
      const x = rand(-94, 94), z = rand(-94, 94), y = this._heightAt(x, z), s = rand(0.7, 1.45);
      dummy.position.set(x, y + 0.05, z); dummy.rotation.set(0, rand(0, Math.PI * 2), rand(-0.12, 0.12)); dummy.scale.set(s, s, s); dummy.updateMatrix(); branches.setMatrixAt(i, dummy.matrix);
    }
    branches.instanceMatrix.needsUpdate = true;
    this.scene.add(branches);
  }

  _spawnTree(x, z) {
    const y = this._heightAt(x, z);
    const group = new THREE.Group();
    const trunkH = rand(3.1, 5.2);
    const trunkMat = new THREE.MeshStandardMaterial({ color: new THREE.Color().setHSL(0.075, 0.42, rand(0.18, 0.27)), roughness: 1, flatShading: true });
    const trunk = new THREE.Mesh(new THREE.CylinderGeometry(0.23, 0.42, trunkH, 7), trunkMat);
    trunk.position.y = trunkH / 2; trunk.castShadow = true; trunk.receiveShadow = true; group.add(trunk);

    const isPine = Math.random() < 0.58;
    if (isPine) {
      const leafMat = new THREE.MeshStandardMaterial({ color: new THREE.Color().setHSL(rand(0.285, 0.33), rand(0.38, 0.58), rand(0.18, 0.29)), roughness: 1, flatShading: true });
      for (let i = 0; i < 4; i++) {
        const tier = new THREE.Mesh(new THREE.ConeGeometry(1.55 - i * 0.2, 2.0, 7), leafMat);
        tier.position.y = trunkH - 0.35 + i * 0.72; tier.rotation.y = rand(0, Math.PI * 2); tier.castShadow = true; group.add(tier);
      }
    } else {
      const leafMat = new THREE.MeshStandardMaterial({ color: new THREE.Color().setHSL(rand(0.26, 0.32), rand(0.42, 0.6), rand(0.2, 0.34)), roughness: 1, flatShading: true });
      for (let i = 0; i < 5; i++) {
        const leaves = new THREE.Mesh(new THREE.IcosahedronGeometry(rand(0.95, 1.65), 1), leafMat);
        const ang = (i / 5) * Math.PI * 2 + rand(-0.35, 0.35);
        leaves.position.set(Math.cos(ang) * rand(0.15, 0.75), trunkH + rand(-0.2, 1.25), Math.sin(ang) * rand(0.15, 0.75));
        leaves.scale.y = rand(0.8, 1.15); leaves.castShadow = true; group.add(leaves);
      }
    }
    group.position.set(x, y, z); group.rotation.y = rand(0, Math.PI * 2); const s = rand(0.88, 1.14); group.scale.set(s, s, s);
    this._treeGroup.add(group);
    this.collidables.push({ mesh: group, type: 'tree', radius: 0.5, height: trunkH, hp: 4, maxHp: 4 });
  }

  _spawnRock(x, z) {
    const y = this._heightAt(x, z), s = rand(0.65, 1.45);
    const group = new THREE.Group();
    const rockMat = new THREE.MeshStandardMaterial({ color: new THREE.Color().setHSL(rand(0.13, 0.18), rand(0.02, 0.08), rand(0.31, 0.43)), roughness: 0.96, metalness: 0.02, flatShading: true });
    const main = new THREE.Mesh(new THREE.DodecahedronGeometry(s, 1), rockMat);
    main.scale.set(1.08, rand(0.7, 1.0), rand(0.82, 1.15)); main.position.y = s * 0.45; main.rotation.set(rand(-0.3, 0.3), rand(0, Math.PI * 2), rand(-0.25, 0.25)); main.castShadow = true; main.receiveShadow = true; group.add(main);
    if (Math.random() < 0.65) {
      const chip = new THREE.Mesh(new THREE.DodecahedronGeometry(s * rand(0.28, 0.48), 0), rockMat);
      chip.position.set(rand(-0.7, 0.7) * s, s * 0.18, rand(-0.65, 0.65) * s); chip.rotation.set(rand(0, 1), rand(0, Math.PI * 2), rand(0, 1)); chip.castShadow = true; group.add(chip);
    }
    group.position.set(x, y, z); this.scene.add(group);
    this.collidables.push({ mesh: group, type: 'rock', radius: s * 0.75, height: s, hp: 5, maxHp: 5 });
  }

  _spawnBush(x, z) {
    const y = this._heightAt(x, z);
    const bush = new THREE.Group();
    const leafMat = new THREE.MeshStandardMaterial({ color: new THREE.Color().setHSL(rand(0.28, 0.34), 0.48, rand(0.2, 0.29)), roughness: 1, flatShading: true });
    for (let i = 0; i < 4; i++) {
      const lobe = new THREE.Mesh(new THREE.IcosahedronGeometry(rand(0.36, 0.58), 1), leafMat);
      const ang = (i / 4) * Math.PI * 2 + rand(-0.3, 0.3);
      lobe.position.set(Math.cos(ang) * rand(0.15, 0.38), rand(0.3, 0.55), Math.sin(ang) * rand(0.15, 0.38)); lobe.scale.y = rand(0.72, 1.0); lobe.castShadow = true; bush.add(lobe);
    }
    const berryMat = new THREE.MeshStandardMaterial({ color: 0x9f2f32, roughness: 0.52 });
    for (let i = 0; i < 5; i++) {
      const berry = new THREE.Mesh(new THREE.SphereGeometry(0.075, 7, 6), berryMat);
      const ang = rand(0, Math.PI * 2); berry.position.set(Math.cos(ang) * rand(0.2, 0.52), rand(0.35, 0.72), Math.sin(ang) * rand(0.2, 0.52)); bush.add(berry);
    }
    bush.position.set(x, y, z); bush.rotation.y = rand(0, Math.PI * 2); this.scene.add(bush);
    this.collidables.push({ mesh: bush, type: 'bush', radius: 0.6, height: 0.8, hp: 1, maxHp: 1 });
  }

  _initControls() {
    this.yaw = 0;
    this.pitch = 0;
    this.velocity = new THREE.Vector3();
    this.onGround = true;

    this._onKeyDown = (e) => {
      this.keys[e.code] = true;
      if (e.code.startsWith('Digit')) {
        const n = Number(e.code.replace('Digit', ''));
        if (n >= 1 && n <= 6) this.selectedSlot = n - 1;
      }
      if (['KeyW', 'KeyA', 'KeyS', 'KeyD', 'Space'].includes(e.code) && this.pointerLocked) e.preventDefault();
    };
    this._onKeyUp = (e) => { this.keys[e.code] = false; };
    window.addEventListener('keydown', this._onKeyDown);
    window.addEventListener('keyup', this._onKeyUp);

    this._onMouseMove = (e) => {
      if (!this.pointerLocked) return;
      const sens = 0.0022;
      this.yaw -= e.movementX * sens;
      this.pitch -= e.movementY * sens;
      this.pitch = clamp(this.pitch, -Math.PI / 2 + 0.05, Math.PI / 2 - 0.05);
    };
    document.addEventListener('mousemove', this._onMouseMove);

    this._onMouseDown = (e) => {
      if (!this.pointerLocked) return;
      if (e.button === 0) { this.mouseDown = true; this._doAction(); }
    };
    this._onMouseUp = () => { this.mouseDown = false; };
    this.renderer.domElement.addEventListener('mousedown', this._onMouseDown);
    window.addEventListener('mouseup', this._onMouseUp);

    this._onCanvasClick = () => {
      if (!this.pointerLocked && this.alive) this.renderer.domElement.requestPointerLock();
    };
    this.renderer.domElement.addEventListener('click', this._onCanvasClick);

    this._onPointerLockChange = () => {
      this.pointerLocked = document.pointerLockElement === this.renderer.domElement;
      this.callbacks.onPointerLock?.(this.pointerLocked);
    };
    document.addEventListener('pointerlockchange', this._onPointerLockChange);
  }

  requestLock() { this.renderer.domElement.requestPointerLock(); }

  _doAction() {
    const now = performance.now();
    if (now - this.lastAttack < 300) return;
    this.lastAttack = now;

    const slot = this.selectedSlot;
    if (slot === 5) {
      if (this.inventory.food > 0 && this.hunger < 100) {
        this.inventory.food--;
        this.hunger = clamp(this.hunger + 35, 0, 100);
        this._msg('Ate berries (+hunger)');
        this._emitState();
      } else this._msg(this.inventory.food <= 0 ? 'No food to eat' : 'Not hungry');
      return;
    }
    if (slot === 3 || slot === 4) { this._tryPlace(slot === 4 ? 'campfire' : 'wall'); return; }
    if (this._attackEnemyAt()) return;

    const ray = new THREE.Raycaster();
    ray.setFromCamera({ x: 0, y: 0 }, this.camera); ray.far = 4.5;
    const hits = ray.intersectObjects(this.collidables.map((c) => c.mesh), true);
    if (!hits.length) return;
    const hit = hits[0];
    let target = null;
    for (const c of this.collidables) if (c.mesh === hit.object || c.mesh.children.includes(hit.object)) { target = c; break; }
    if (!target) return;

    const isAxe = slot === 1, isPick = slot === 2;
    const dmg = isAxe || isPick ? 2 : 1;
    const rightTool = (target.type === 'tree' && isAxe) || (target.type === 'rock' && isPick) || target.type === 'bush';
    target.hp -= rightTool ? dmg * 1.5 : dmg;
    target.mesh.scale.multiplyScalar(0.92);
    setTimeout(() => { if (target?.mesh) target.mesh.scale.multiplyScalar(1 / 0.92); }, 80);
    if (target.hp <= 0) this._harvest(target);
  }

  _harvest(target) {
    let drops = {};
    if (target.type === 'tree') drops = { wood: Math.floor(rand(2, 4)) };
    else if (target.type === 'rock') drops = { stone: Math.floor(rand(2, 4)) };
    else if (target.type === 'bush') drops = { food: Math.floor(rand(1, 3)) };
    for (const k in drops) this.inventory[k] = (this.inventory[k] || 0) + drops[k];
    this._msg(Object.entries(drops).map(([k, v]) => `+${v} ${k}`).join('  '));
    if (target.type === 'tree' && !this.unlocked.axe) { this.unlocked.axe = true; this._msg('Crafted a stone axe! (slot 2)'); }
    if (target.type === 'rock' && !this.unlocked.pickaxe) { this.unlocked.pickaxe = true; this._msg('Crafted a stone pickaxe! (slot 3)'); }
    this.scene.remove(target.mesh);
    this.collidables = this.collidables.filter((c) => c !== target);
    this._emitState();
  }

  _tryPlace(kind) {
    const cost = kind === 'campfire' ? { wood: 3, stone: 2 } : { wood: 2 };
    for (const k in cost) if ((this.inventory[k] || 0) < cost[k]) { this._msg(`Need ${cost[k]} ${k} to build ${kind}`); return; }
    const dir = new THREE.Vector3(); this.camera.getWorldDirection(dir);
    const pos = this.camera.position.clone().add(dir.multiplyScalar(3)); pos.y = this._heightAt(pos.x, pos.z);

    if (kind === 'wall') {
      const block = new THREE.Mesh(new THREE.BoxGeometry(2, 2, 0.3), new THREE.MeshStandardMaterial({ color: 0x8a6a3a, roughness: 1 }));
      block.position.set(pos.x, pos.y + 1, pos.z); block.lookAt(this.camera.position.x, pos.y + 1, this.camera.position.z); block.castShadow = true; block.receiveShadow = true;
      this.scene.add(block); this.placedBlocks.push(block); this.collidables.push({ mesh: block, type: 'wall', radius: 1, height: 2, hp: 10, maxHp: 10, solid: true });
    } else {
      const fire = new THREE.Group();
      const logs = new THREE.Mesh(new THREE.CylinderGeometry(0.4, 0.4, 0.2, 6), new THREE.MeshStandardMaterial({ color: 0x3a2a1a }));
      logs.position.y = 0.1; fire.add(logs);
      const flame = new THREE.Mesh(new THREE.ConeGeometry(0.3, 0.8, 6), new THREE.MeshBasicMaterial({ color: 0xff8833 }));
      flame.position.y = 0.5; fire.add(flame);
      const light = new THREE.PointLight(0xff8833, 2, 12, 2); light.position.y = 1; fire.add(light);
      fire.position.set(pos.x, pos.y, pos.z); this.scene.add(fire); this.placedBlocks.push(fire); fire.userData.light = light; fire.userData.flame = flame;
      this.campfires = this.campfires || []; this.campfires.push(fire);
    }
    for (const k in cost) this.inventory[k] -= cost[k];
    this._msg(`Built ${kind}`); this._emitState();
  }

  _spawnEnemy() {
    const ang = rand(0, Math.PI * 2), dist = rand(25, 40);
    const x = this.camera.position.x + Math.cos(ang) * dist, z = this.camera.position.z + Math.sin(ang) * dist, y = this._heightAt(x, z);
    const enemy = new THREE.Group();
    const body = new THREE.Mesh(new THREE.CapsuleGeometry(0.35, 0.9, 4, 8), new THREE.MeshStandardMaterial({ color: 0x3a5a3a, roughness: 1, flatShading: true }));
    body.position.y = 0.9; body.castShadow = true; enemy.add(body);
    const head = new THREE.Mesh(new THREE.SphereGeometry(0.28, 8, 8), new THREE.MeshStandardMaterial({ color: 0x6a8a5a, roughness: 1, flatShading: true }));
    head.position.y = 1.7; head.castShadow = true; enemy.add(head);
    const eyeMat = new THREE.MeshBasicMaterial({ color: 0xff3300 });
    const e1 = new THREE.Mesh(new THREE.SphereGeometry(0.05, 6, 6), eyeMat); e1.position.set(-0.1, 1.72, 0.24);
    const e2 = e1.clone(); e2.position.x = 0.1; enemy.add(e1, e2);
    enemy.position.set(x, y, z); this.scene.add(enemy);
    this.enemies.push({ mesh: enemy, hp: 30, maxHp: 30, lastAttack: 0, speed: rand(2.2, 3.2) });
  }

  _updateEnemies(dt) {
    const playerPos = this.camera.position;
    for (const e of this.enemies) {
      const dir = new THREE.Vector3().subVectors(playerPos, e.mesh.position); dir.y = 0;
      const dist = dir.length(); dir.normalize();
      if (dist > 1.2) { e.mesh.position.x += dir.x * e.speed * dt; e.mesh.position.z += dir.z * e.speed * dt; }
      e.mesh.position.y = this._heightAt(e.mesh.position.x, e.mesh.position.z);
      e.mesh.lookAt(playerPos.x, e.mesh.position.y, playerPos.z);
      if (dist < 1.6) {
        const now = performance.now();
        if (now - e.lastAttack > 1000) { e.lastAttack = now; this.health -= 8; this._msg('You took damage!'); if (this.health <= 0) this._die(); }
      }
    }
  }

  _attackEnemyAt() {
    if (this.selectedSlot === 3 || this.selectedSlot === 4 || this.selectedSlot === 5) return false;
    const ray = new THREE.Raycaster(); ray.setFromCamera({ x: 0, y: 0 }, this.camera); ray.far = 4;
    const hits = ray.intersectObjects(this.enemies.map((e) => e.mesh), true);
    if (!hits.length) return false;
    let target = null;
    for (const e of this.enemies) if (e.mesh === hits[0].object || e.mesh.children.includes(hits[0].object)) { target = e; break; }
    if (!target) return false;
    target.hp -= 10;
    const dir = new THREE.Vector3().subVectors(target.mesh.position, this.camera.position).normalize();
    target.mesh.position.add(dir.multiplyScalar(0.5));
    if (target.hp <= 0) {
      this.scene.remove(target.mesh); this.enemies = this.enemies.filter((x) => x !== target); this._msg('Enemy defeated!');
      if (Math.random() < 0.5) { this.inventory.food = (this.inventory.food || 0) + 1; this._msg('+1 food'); }
      this._emitState();
    }
    this.lastAttack = performance.now();
    return true;
  }

  _updateTime(dt) {
    this.timeOfDay += dt / 120;
    if (this.timeOfDay >= 1) { this.timeOfDay -= 1; this.day++; this._msg(`Day ${this.day} — survive!`); }
    const t = this.timeOfDay;
    const sunAngle = t * Math.PI * 2 - Math.PI / 2;
    this.sun.position.set(Math.cos(sunAngle) * 60, Math.sin(sunAngle) * 60 + 5, 30);
    const brightness = clamp(Math.sin(t * Math.PI), 0, 1);
    const isNight = brightness < 0.15;
    this.sun.intensity = brightness * 1.35 + 0.025;
    this.ambient.intensity = 0.1 + brightness * 0.3;
    this.hemisphere.intensity = 0.14 + brightness * 0.42;
    this.renderer.toneMappingExposure = 0.68 + brightness * 0.28;

    const dayTop = new THREE.Color(0x587f9b), nightTop = new THREE.Color(0x050911), dayBot = new THREE.Color(0xb5c8cf), nightBot = new THREE.Color(0x10151b);
    this.sky.material.uniforms.topColor.value.copy(nightTop).lerp(dayTop, brightness);
    this.sky.material.uniforms.bottomColor.value.copy(nightBot).lerp(dayBot, brightness);
    this.scene.background.copy(this.sky.material.uniforms.bottomColor.value);
    this.scene.fog.color.copy(this.scene.background);
    this.scene.fog.near = 27 + brightness * 7;
    this.scene.fog.far = 92 + brightness * 26;

    const nightAmount = 1 - brightness;
    if (this.starMaterial) this.starMaterial.opacity = clamp((nightAmount - 0.45) * 1.75, 0, 0.92);
    if (this.moon && this.moonMaterial) {
      const moonAngle = sunAngle + Math.PI;
      this.moon.position.set(Math.cos(moonAngle) * 145, Math.sin(moonAngle) * 100 + 35, -115);
      this.moonMaterial.opacity = clamp((nightAmount - 0.5) * 1.7, 0, 0.86);
    }

    this.playerLight.intensity = isNight ? 1.2 : 0;
    this.playerLight.position.copy(this.camera.position); this.playerLight.position.y += 0.5;

    if (this.campfires) for (const f of this.campfires) {
      f.userData.light.intensity = 1.5 + Math.sin(performance.now() * 0.01) * 0.3 + Math.random() * 0.2;
      f.userData.flame.scale.y = 1 + Math.sin(performance.now() * 0.015) * 0.2;
    }

    if (isNight) {
      this._enemySpawnTimer = (this._enemySpawnTimer || 0) + dt;
      const maxEnemies = 3 + this.day;
      if (this._enemySpawnTimer > 4 && this.enemies.length < maxEnemies) { this._enemySpawnTimer = 0; this._spawnEnemy(); }
    } else if (this.enemies.length > 0 && Math.random() < 0.01) {
      const e = this.enemies.pop(); this.scene.remove(e.mesh);
    }
  }

  _updatePlayer(dt) {
    if (!this.alive) return;
    const speed = this.keys.ShiftLeft && this.stamina > 0 ? 8 : 4.5;
    const sprinting = this.keys.ShiftLeft && this.stamina > 0 && (this.keys.KeyW || this.keys.KeyA || this.keys.KeyS || this.keys.KeyD);
    if (sprinting) this.stamina = clamp(this.stamina - dt * 15, 0, 100); else this.stamina = clamp(this.stamina + dt * 8, 0, 100);

    const forward = new THREE.Vector3(-Math.sin(this.yaw), 0, -Math.cos(this.yaw));
    const right = new THREE.Vector3(Math.cos(this.yaw), 0, -Math.sin(this.yaw));
    const move = new THREE.Vector3();
    if (this.keys.KeyW) move.add(forward); if (this.keys.KeyS) move.sub(forward); if (this.keys.KeyD) move.add(right); if (this.keys.KeyA) move.sub(right);
    const moving = move.lengthSq() > 0;
    if (moving) move.normalize().multiplyScalar(speed * dt);

    const px = this.camera.position.x + move.x, pz = this.camera.position.z + move.z;
    let blockedX = false, blockedZ = false;
    for (const c of this.collidables) {
      if (!c.solid && c.type !== 'rock' && c.type !== 'wall') continue;
      const dx = px - c.mesh.position.x, dz = this.camera.position.z - c.mesh.position.z;
      if (Math.hypot(dx, dz) < c.radius + 0.5) blockedX = true;
      const dx2 = this.camera.position.x - c.mesh.position.x, dz2 = pz - c.mesh.position.z;
      if (Math.hypot(dx2, dz2) < c.radius + 0.5) blockedZ = true;
    }
    if (!blockedX) this.camera.position.x = px;
    if (!blockedZ) this.camera.position.z = pz;
    this.camera.position.x = clamp(this.camera.position.x, -95, 95);
    this.camera.position.z = clamp(this.camera.position.z, -95, 95);

    const groundY = this._heightAt(this.camera.position.x, this.camera.position.z) + 1.7;
    this.velocity.y -= 18 * dt;
    this.camera.position.y += this.velocity.y * dt;
    if (this.camera.position.y <= groundY) { this.camera.position.y = groundY; this.velocity.y = 0; this.onGround = true; }
    if (this.keys.Space && this.onGround) { this.velocity.y = 7; this.onGround = false; }

    this.camera.rotation.order = 'YXZ';
    this.camera.rotation.y = this.yaw;
    this.camera.rotation.x = this.pitch;
    this._updatePlayerModel(dt, moving, sprinting);

    this.hunger = clamp(this.hunger - dt * 0.6, 0, 100);
    if (this.hunger <= 0) { this.health = clamp(this.health - dt * 4, 0, 100); if (this.health <= 0) this._die(); }
    else if (this.hunger > 60 && this.health < this.maxHealth) this.health = clamp(this.health + dt * 1.5, 0, this.maxHealth);
  }

  _animate() {
    if (this.disposed) return;
    const dt = Math.min(this.clock.getDelta(), 0.05);
    if (this.pointerLocked && this.alive) {
      this._updatePlayer(dt);
      if (this.mouseDown && performance.now() - this.lastAttack > 400) this._doAction();
      this._updateEnemies(dt);
    }
    this._updateTime(dt);
    this._emitAccum = (this._emitAccum || 0) + dt;
    if (this._emitAccum > 0.2) { this._emitAccum = 0; this._emitState(); }
    this.renderer.render(this.scene, this.camera);
  }

  _emitState() {
    this.callbacks.onState?.({
      health: Math.round(this.health), maxHealth: this.maxHealth, hunger: Math.round(this.hunger), stamina: Math.round(this.stamina), day: this.day,
      timeOfDay: this.timeOfDay, inventory: { ...this.inventory }, selectedSlot: this.selectedSlot, unlocked: { ...this.unlocked }, alive: this.alive, enemyCount: this.enemies.length,
    });
  }

  _msg(text) { this.callbacks.onMessage?.(text); }

  _die() {
    if (!this.alive) return;
    this.alive = false;
    if (document.pointerLockElement) document.exitPointerLock();
    this.callbacks.onDeath?.({ day: this.day });
  }

  selectSlot(i) { this.selectedSlot = i; this._emitState(); }

  dispose() {
    this.disposed = true;
    this.renderer.setAnimationLoop(null);
    window.removeEventListener('resize', this._onResize);
    window.removeEventListener('keydown', this._onKeyDown);
    window.removeEventListener('keyup', this._onKeyUp);
    document.removeEventListener('mousemove', this._onMouseMove);
    window.removeEventListener('mouseup', this._onMouseUp);
    document.removeEventListener('pointerlockchange', this._onPointerLockChange);
    this.renderer.domElement.removeEventListener('mousedown', this._onMouseDown);
    this.renderer.domElement.removeEventListener('click', this._onCanvasClick);
    this.renderer.dispose();
    if (this.renderer.domElement.parentNode) this.renderer.domElement.parentNode.removeChild(this.renderer.domElement);
  }
}
