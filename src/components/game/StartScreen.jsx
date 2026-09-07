import React from 'react';

export default function StartScreen({ onStart, lastDeath }) {
  return (
    <div
      className="absolute inset-0 flex items-center justify-center overflow-hidden bg-[#090c0a] px-6 py-10 font-mono text-white"
      style={{
        backgroundImage:
          'radial-gradient(circle at 50% 25%, rgba(71,95,77,0.30), transparent 34%), linear-gradient(180deg, #121813 0%, #090c0a 58%, #050706 100%)',
      }}
    >
      <div className="pointer-events-none absolute inset-0 opacity-20" style={{ backgroundImage: 'linear-gradient(rgba(255,255,255,0.025) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.02) 1px, transparent 1px)', backgroundSize: '48px 48px' }} />
      <div className="pointer-events-none absolute inset-x-0 top-0 h-px bg-gradient-to-r from-transparent via-amber-200/30 to-transparent" />

      <div className="relative w-full max-w-3xl">
        <div className="mb-9 text-center">
          <div className="mb-3 text-[9px] font-bold uppercase tracking-[0.48em] text-amber-200/55">Hardcore Wilderness Survival</div>
          <h1 className="text-5xl font-black uppercase tracking-[0.14em] text-stone-100 sm:text-7xl">
            Wild<span className="text-amber-300">Bound</span>
          </h1>
          <div className="mx-auto mt-4 h-px w-28 bg-gradient-to-r from-transparent via-stone-300/40 to-transparent" />
          <p className="mx-auto mt-5 max-w-lg text-xs leading-6 text-stone-300/55">
            Gather what the land gives you. Build before the light disappears. Every night gets harder.
          </p>
        </div>

        {lastDeath && (
          <div className="mx-auto mb-6 max-w-xl border border-red-500/25 bg-red-950/20 px-5 py-3 text-center">
            <div className="text-[8px] font-bold uppercase tracking-[0.3em] text-red-300/55">Run Ended</div>
            <p className="mt-1 text-sm font-bold text-red-200">You survived until Day {lastDeath.day}.</p>
          </div>
        )}

        <div className="grid gap-px overflow-hidden border border-white/10 bg-white/10 sm:grid-cols-3">
          <InfoPanel title="Movement" lines={[['WASD', 'Move'], ['SHIFT', 'Sprint'], ['SPACE', 'Jump']]} />
          <InfoPanel title="Survival" lines={[['LMB', 'Harvest / attack'], ['1—6', 'Select hotbar'], ['ESC', 'Release mouse']]} />
          <InfoPanel title="Objective" lines={[['WOOD', 'Tools & shelter'], ['STONE', 'Tools & fire'], ['FOOD', 'Stay alive']]} />
        </div>

        <button
          onClick={onStart}
          className="group mt-5 w-full border border-amber-200/45 bg-amber-300 px-6 py-4 text-sm font-black uppercase tracking-[0.28em] text-[#11130f] transition hover:bg-amber-200 hover:shadow-[0_0_40px_rgba(252,211,77,0.16)] focus:outline-none focus:ring-2 focus:ring-amber-200/60"
        >
          {lastDeath ? 'Enter Again' : 'Enter The Wild'}
          <span className="ml-3 inline-block transition-transform group-hover:translate-x-1">→</span>
        </button>

        <div className="mt-4 flex flex-wrap items-center justify-center gap-x-6 gap-y-2 text-[8px] uppercase tracking-[0.2em] text-white/25">
          <span>Harvest trees to unlock the axe</span>
          <span>Harvest stone to unlock the pickaxe</span>
          <span>Night brings threats</span>
        </div>
      </div>
    </div>
  );
}

function InfoPanel({ title, lines }) {
  return (
    <div className="bg-[#0b0f0c]/90 p-4">
      <div className="mb-3 text-[8px] font-bold uppercase tracking-[0.28em] text-amber-200/55">{title}</div>
      <div className="space-y-2">
        {lines.map(([key, value]) => (
          <div key={`${key}-${value}`} className="flex items-center justify-between gap-4">
            <span className="text-[9px] font-bold tracking-[0.12em] text-stone-200/75">{key}</span>
            <span className="text-[9px] text-stone-400/55">{value}</span>
          </div>
        ))}
      </div>
    </div>
  );
}
