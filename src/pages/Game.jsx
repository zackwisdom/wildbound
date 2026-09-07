import React, { useState, useEffect, useRef, useCallback } from 'react';
import { SurvivalGame } from '../game/SurvivalGame.js';
import HUD from '../components/game/HUD.jsx';
import StartScreen from '../components/game/StartScreen.jsx';

export default function Game() {
  const containerRef = useRef(null);
  const gameRef = useRef(null);
  const [state, setState] = useState(null);
  const [messages, setMessages] = useState([]);
  const [started, setStarted] = useState(false);
  const [pointerLocked, setPointerLocked] = useState(false);
  const [death, setDeath] = useState(null);

  const pushMessage = useCallback((text) => {
    const id = Date.now() + Math.random();
    setMessages((m) => [...m.slice(-4), { id, text }]);
    setTimeout(() => {
      setMessages((m) => m.filter((x) => x.id !== id));
    }, 3000);
  }, []);

  const startGame = useCallback(() => {
    if (gameRef.current) {
      gameRef.current.dispose();
      gameRef.current = null;
    }
    setDeath(null);
    setStarted(true);
    setState(null);

    requestAnimationFrame(() => {
      if (!containerRef.current) return;
      const game = new SurvivalGame(containerRef.current, {
        onState: (s) => setState(s),
        onMessage: (text) => pushMessage(text),
        onPointerLock: (locked) => setPointerLocked(locked),
        onDeath: (info) => {
          setDeath(info);
          setStarted(false);
        },
      });
      gameRef.current = game;
      game.requestLock();
    });
  }, [pushMessage]);

  useEffect(() => {
    return () => {
      if (gameRef.current) gameRef.current.dispose();
    };
  }, []);

  return (
    <div className="relative h-screen w-screen overflow-hidden bg-[#060806]">
      <div ref={containerRef} className="absolute inset-0" />

      {started && (
        <>
          <div
            className="pointer-events-none absolute inset-0"
            style={{ background: 'radial-gradient(circle at center, transparent 38%, rgba(1,4,2,0.12) 68%, rgba(0,0,0,0.46) 100%)' }}
          />
          <div className="pointer-events-none absolute inset-x-0 top-0 h-28 bg-gradient-to-b from-black/20 to-transparent" />
        </>
      )}

      {started && state && <HUD state={state} />}

      {started && (
        <div className="pointer-events-none absolute left-1/2 top-24 -translate-x-1/2 space-y-1.5 text-center font-mono">
          {messages.map((m) => (
            <div key={m.id} className="border border-white/10 bg-[#090c0a]/80 px-4 py-2 text-[10px] font-bold uppercase tracking-[0.12em] text-stone-200 shadow-xl shadow-black/25 backdrop-blur-md">
              {m.text}
            </div>
          ))}
        </div>
      )}

      {started && state?.alive && !pointerLocked && (
        <div className="pointer-events-none absolute inset-0 flex items-center justify-center bg-black/50 font-mono backdrop-blur-[2px]">
          <div className="border border-white/10 bg-[#090c0a]/90 px-8 py-6 text-center text-white shadow-2xl shadow-black/50">
            <p className="text-xs font-black uppercase tracking-[0.28em] text-stone-100">Click to resume</p>
            <p className="mt-2 text-[9px] uppercase tracking-[0.16em] text-white/35">ESC releases the mouse</p>
          </div>
        </div>
      )}

      {!started && <StartScreen onStart={startGame} lastDeath={death} />}
    </div>
  );
}
