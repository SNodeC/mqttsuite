const state = document.getElementById("connection-state");
const events = document.getElementById("events");

const units = {
  temperature_c: " °C",
  ph_level: "",
  tds_ppm: " ppm",
  turbidity_ntu: " NTU",
  ph_board_temperature_c: " °C",
};

function setConnection(text, cssClass) {
  state.textContent = text;
  state.className = `status ${cssClass}`;
}

function formatNumber(value) {
  if (value === null || value === undefined || Number.isNaN(Number(value))) {
    return "—";
  }
  return Number(value).toFixed(3).replace(/\.000$/, "");
}

function addEventLine(type, data) {
  const line = document.createElement("pre");
  line.textContent = `${new Date().toLocaleTimeString()} ${type}\n${JSON.stringify(data, null, 2)}`;
  events.prepend(line);
  while (events.children.length > 20) {
    events.removeChild(events.lastChild);
  }
}

function updateScalar(data) {
  const value = document.getElementById(data.field);
  const meta = document.getElementById(`${data.field}_meta`);
  if (!value || !meta) {
    return;
  }

  value.textContent = `${formatNumber(data.value)}${units[data.field] ?? ""}`;
  meta.textContent = `${data.device_id}, fPort ${data.f_port}, ${data.received_at}`;
}

function updateGps(data) {
  document.getElementById("gps_latitude").textContent = formatNumber(data.latitude);
  document.getElementById("gps_longitude").textContent = formatNumber(data.longitude);
  document.getElementById("gps_altitude").textContent = data.altitude === null ? "—" : formatNumber(data.altitude);
  document.getElementById("gps_hdop").textContent = data.hdop === null ? "—" : formatNumber(data.hdop);
  document.getElementById("gps_meta").textContent = `${data.device_id}, ${data.received_at}`;

  const map = document.getElementById("gps_map");
  map.href = `https://www.openstreetmap.org/?mlat=${data.latitude}&mlon=${data.longitude}#map=17/${data.latitude}/${data.longitude}`;
  map.classList.remove("hidden");
}

function applyInitialLatest(latest) {
  if (!latest) {
    return;
  }

  for (const key of ["temperature", "ph", "tds", "turbidity", "board_temperature"]) {
    if (latest[key]) {
      updateScalar(latest[key]);
    }
  }

  if (latest.gps) {
    updateGps(latest.gps);
  }
}

const source = new EventSource("/api/water-buoy/events");

source.onopen = () => setConnection("connected", "status-ok");
source.onerror = () => setConnection("reconnecting", "status-waiting");

source.addEventListener("ui-initialize", (event) => {
  const data = JSON.parse(event.data);
  applyInitialLatest(data.latest);
  addEventLine("ui-initialize", data);
});

source.addEventListener("scalar-measurement", (event) => {
  const data = JSON.parse(event.data);
  updateScalar(data);
  addEventLine("scalar-measurement", data);
});

source.addEventListener("gps-position", (event) => {
  const data = JSON.parse(event.data);
  updateGps(data);
  addEventLine("gps-position", data);
});

source.addEventListener("storage-error", (event) => {
  const data = JSON.parse(event.data);
  setConnection("storage error", "status-error");
  addEventLine("storage-error", data);
});
