/* global echarts */

const metaEl = document.getElementById("meta");
const statsEl = document.getElementById("stats");
const chartEl = document.getElementById("chart");
const chart = echarts.init(chartEl, null, { renderer: "canvas" });

const WINDOW_SEC = 12.0;
const series = {
  ax: [],
  ay: [],
  az: [],
  norm: [],
};

let lastSample = null;
let samples = 0;
let t0us = null;
let lastFlush = performance.now();
const pending = [];

function pushPoint(arr, x, y) {
  arr.push([x, y]);
  // Trim by time window
  const cutoff = x - WINDOW_SEC;
  while (arr.length > 2 && arr[0][0] < cutoff) arr.shift();
}

function render() {
  const now = performance.now();
  // throttle chart updates ~20Hz
  if (now - lastFlush < 50) return;
  lastFlush = now;

  chart.setOption(
    {
      animation: false,
      grid: { left: 56, right: 18, top: 28, bottom: 34 },
      legend: { top: 6, textStyle: { color: "rgba(255,255,255,0.75)" } },
      xAxis: {
        type: "value",
        name: "s",
        axisLabel: { color: "rgba(255,255,255,0.6)" },
        splitLine: { lineStyle: { color: "rgba(255,255,255,0.10)" } },
      },
      yAxis: {
        type: "value",
        name: "mg",
        axisLabel: { color: "rgba(255,255,255,0.6)" },
        splitLine: { lineStyle: { color: "rgba(255,255,255,0.10)" } },
      },
      series: [
        { name: "ax", type: "line", showSymbol: false, data: series.ax, lineStyle: { width: 1.5, color: "#ffcf33" } },
        { name: "ay", type: "line", showSymbol: false, data: series.ay, lineStyle: { width: 1.5, color: "#4dd3ff" } },
        { name: "az", type: "line", showSymbol: false, data: series.az, lineStyle: { width: 1.5, color: "#7CFF6B" } },
        { name: "|a|", type: "line", showSymbol: false, data: series.norm, lineStyle: { width: 1.0, color: "#ff5c7a", opacity: 0.65 } },
      ],
      tooltip: { trigger: "axis" },
    },
    { notMerge: true }
  );

  if (lastSample) {
    statsEl.textContent =
      `ax=${lastSample.ax_mg} ay=${lastSample.ay_mg} az=${lastSample.az_mg} |a|=${lastSample.norm_mg.toFixed(1)} ` +
      `t=${lastSample.temp_c.toFixed(2)}C rh=${lastSample.rh_pct.toFixed(2)}%  samples=${samples}`;
  }
}

function flushPending() {
  if (pending.length === 0) return;
  while (pending.length) {
    const s = pending.shift();
    if (t0us === null) t0us = s.t_us;
    const x = (s.t_us - t0us) / 1e6;
    pushPoint(series.ax, x, s.ax_mg);
    pushPoint(series.ay, x, s.ay_mg);
    pushPoint(series.az, x, s.az_mg);
    pushPoint(series.norm, x, s.norm_mg);
    lastSample = s;
    samples += 1;
  }
  render();
}

setInterval(flushPending, 30);
window.addEventListener("resize", () => chart.resize());

function connect() {
  const ws = new WebSocket(`ws://${location.host}/ws`);
  metaEl.textContent = "connecting websocket...";

  ws.onopen = () => {
    metaEl.textContent = "connected";
    // Send keepalive pings so server's receive_text loop doesn't block forever.
    setInterval(() => {
      if (ws.readyState === WebSocket.OPEN) ws.send("ping");
    }, 1000);
  };

  ws.onmessage = (ev) => {
    try {
      pending.push(JSON.parse(ev.data));
    } catch (_) {
      // ignore
    }
  };

  ws.onclose = () => {
    metaEl.textContent = "disconnected; retrying...";
    setTimeout(connect, 800);
  };

  ws.onerror = () => {
    // Let onclose handle retry.
  };
}

connect();

