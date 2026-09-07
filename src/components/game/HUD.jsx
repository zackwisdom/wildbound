import React from 'react';

export default function HUD({ state }) {
  if (!state) return null;
  const { health, maxHealth, hunger, stamina, day, timeOfDay, inventory, selectedSlot, unlocked, enemyCount } = state;
  const isNight = timeOfDay < 0.22 || timeOfDay > 0.78;
  const critical = health <= 30 || hunger <= 20;

  const slots = [
    { key: 'hand', label: 'Hands', icon: '✋', n: 1 },
    { key: 'axe', label: 'Axe', icon: '🪓', n: 2, locked: !unlocked?.axe },
    { key: 'pick', label: 'Pick', icon: '⛏️', n: 3, locked: !unlocked?.pickaxe },
    { key: 'wall', label: 'Wall', icon: '🧱', n: 4 },
    { key: 'fire', label: 'Fire', icon: '🔥', n: 5 },
    { key: 'eat', label: 'Eat', icon: '🍓', n: 6, count: inventory?.food },
  ];

  return (
    <div className="pointer-events-none absolute inset-0 select-none font-mono text-white">
      <div className="absolute left-5 top-5">
        <div className="text-[10px] font-bold uppercase tracking-[0.34em] text-white/45">WildBound</div>
        <div className="mt-1 text-[9px] uppercase tracking-[0.2em] text-white/25">Wilderness survival protocol</div>
      </div>

      <div className="absolute left-1/2 top-1/2 -translate-x-1/2 -translate-y-1/2">
        <div className="relative h-5 w-5 opacity-80 drop-shadow-[0_1px_2px_rgba(0,0,0,0.9)]">
          <div className="absolute left-1/2 top-0 h-1.5 w-px -translate-x-1/2 bg-white" />
          <div className="absolute bottom-0 left-1/2 h-1.5 w-px -translate-x-1/2 bg-white" />
          <div className="absolute left-0 top-1/2 h-px w-1.5 -translate-y-1/2 bg-white" />
          <div className="absolute right-0 top-1/2 h-px w-1.5 -translate-y-1/2 bg-white" />
        </div>
      </div>

      <div className="absolute left-1/2 top-4 -translate-x-1/2">
        <div className="flex items-stretch overflow-hidden border border-white/10 bg-[#0b0e0d]/70 shadow-2xl shadow-black/30 backdrop-blur-md">
          <div className="border-r border-white/10 px-5 py-2.5 text-center">
            <div className="text-[8px] uppercase tracking-[0.3em] text-white/35">Day</div>
            <div className="text-sm font-bold tracking-[0.16em] text-stone-100">{String(day).padStart(2, '0')}</div>
          </div>
          <div className="px-5 py-2.5 text-center">
            <div className="text-[8px] uppercase tracking-[0.3em] text-white/35">Cycle</div>
            <div className={`text-[11px] font-bold uppercase tracking-[0.18em] ${isNight ? 'text-slate-300' : 'text-amber-200'}`}>
              {isNight ? 'Night' : 'Daylight'}
            </div>
          </div>
          {isNight && enemyCount > 0 && (
            <div className="border-l border-red-400/20 bg-red-950/30 px-4 py-2.5 text-center">
              <div className="text-[8px] uppercase tracking-[0.3em] text-red-300/55">Threats</div>
              <div className="text-[11px] font-bold tracking-[0.18em] text-red-300">{enemyCount}</div>
            </div>
          )}
        </div>
      </div>

      <div className={`absolute bottom-5 left-5 w-64 border bg-[#0b0e0d]/72 p-3.5 shadow-2xl shadow-black/35 backdrop-blur-md ${critical ? 'border-red-500/30' : 'border-white/10'}`}>
        <div className="mb-3 flex items-center justify-between border-b border-white/10 pb-2">
          <span className="text-[9px] font-bold uppercase tracking-[0.26em] text-white/50">Condition</span>
          {critical && <span className="animate-pulse text-[8px] font-bold uppercase tracking-[0.2em] text-red-300">Critical</span>}
        </div>
        <div className="space-y-2.5">
          <VitalBar label="Health" value={health} max={maxHealth} barClass="bg-red-500" />
          <VitalBar label="Hunger" value={hunger} max={100} barClass="bg-amber-400" />
          <VitalBar label="Stamina" value={stamina} max={100} barClass="bg-emerald-500" />
        </div>
      </div>

      <div className="absolute bottom-5 right-5 min-w-52 border border-white/10 bg-[#0b0e0d]/72 p-3.5 shadow-2xl shadow-black/35 backdrop-blur-md">
        <div className="mb-3 border-b border-white/10 pb-2 text-[9px] font-bold uppercase tracking-[0.26em] text-white/50">Pack</div>
        <div className="grid grid-cols-3 gap-4 text-center">
          <Resource icon="🪵" label="Wood" value={inventory?.wood ?? 0} />
          <Resource icon="🪨" label="Stone" value={inventory?.stone ?? 0} />
          <Resource icon="🍓" label="Food" value={inventory?.food ?? 0} />
        </div>
      </div>

      <div className="absolute bottom-5 left-1/2 flex -translate-x-1/2 gap-1.5">
        {slots.map((s) => {
          const active = selectedSlot === s.n - 1;
          return (
            <div
              key={s.key}
              className={`relative flex h-[58px] w-[58px] flex-col items-center justify-center border shadow-lg backdrop-blur-md transition-all ${active ? 'border-amber-300/80 bg-amber-200/15 shadow-amber-950/30' : 'border-white/10 bg-[#0b0e0d]/68 shadow-black/30'} ${s.locked ? 'opacity-35' : ''}`}
            >
              <span className="text-[19px] leading-none">{s.locked ? '🔒' : s.icon}</span>
              <span className="mt-1 text-[7px] font-bold uppercase tracking-[0.12em] text-white/45">{s.label}</span>
              <span className="absolute left-1 top-0.5 text-[8px] text-white/30">{s.n}</span>
              {s.count !== undefined && <span className="absolute right-1 top-0.5 text-[8px] font-bold text-white/70">{s.count}</span>}
            </div>
          );
        })}
      </div>
    </div>
  );
}

function VitalBar({ label, value, max, barClass }) {
  const pct = Math.max(0, Math.min(100, (value / max) * 100));
  const danger = pct <= 25;
  return (
    <div>
      <div className="mb-1 flex items-end justify-between">
        <span className="text-[8px] font-bold uppercase tracking-[0.2em] text-white/45">{label}</span>
        <span className={`text-[9px] font-bold tabular-nums ${danger ? 'text-red-300' : 'text-white/70'}`}>{Math.round(value)}</span>
      </div>
      <div className="h-1.5 w-full overflow-hidden bg-black/65 ring-1 ring-white/5">
        <div className={`h-full ${barClass} transition-all duration-200`} style={{ width: `${pct}%` }} />
      </div>
    </div>
  );
}

function Resource({ icon, label, value }) {
  return (
    <div>
      <div className="text-base leading-none">{icon}</div>
      <div className="mt-1 text-xs font-bold tabular-nums text-stone-100">{value}</div>
      <div className="mt-0.5 text-[7px] uppercase tracking-[0.14em] text-white/30">{label}</div>
    </div>
  );
}
