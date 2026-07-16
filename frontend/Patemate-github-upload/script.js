const breedProfiles = {
  golden: { name: "金毛", weightRange: [25, 34], walk: [60, 90], calories: [350, 500], speed: "3.5-6 km/h", tip: "注意髋关节和体重管理", intro: "金毛亲人且运动需求稳定，适合中等到较长时间的规律散步。", sizeFactor: 0.8 },
  labrador: { name: "拉布拉多", weightRange: [25, 36], walk: [60, 90], calories: [360, 520], speed: "3.5-6 km/h", tip: "控制零食和体重", intro: "拉布拉多食欲强、耐力好，适合规律运动和体重管理。", sizeFactor: 0.85 },
  corgi: { name: "柯基", weightRange: [10, 12], walk: [45, 60], calories: [200, 280], speed: "3-5 km/h", tip: "避开高温时段", intro: "柯基腿短背长，散步宜稳定，不建议频繁跳跃或突然冲刺。", sizeFactor: 1 },
  husky: { name: "哈士奇", weightRange: [20, 27], walk: [90, 120], calories: [400, 600], speed: "4-8 km/h", tip: "适合更长时间运动", intro: "哈士奇精力旺盛，更适合较长时间散步和耐力型活动。", sizeFactor: 0.85 },
  poodle: { name: "泰迪", weightRange: [3, 6], walk: [30, 45], calories: [100, 150], speed: "2.5-4.5 km/h", tip: "关注膝关节状态", intro: "泰迪体型较小，运动量不用太大，但需要保持日常活动频率。", sizeFactor: 1.2 },
  border: { name: "边牧", weightRange: [14, 20], walk: [90, 120], calories: [300, 450], speed: "4-7 km/h", tip: "加入训练任务更合适", intro: "边牧聪明且精力高，单纯散步之外可以加入寻回和指令训练。", sizeFactor: 1 },
  shiba: { name: "柴犬", weightRange: [8, 11], walk: [45, 70], calories: [180, 260], speed: "3-5.5 km/h", tip: "保持牵引和召回训练", intro: "柴犬独立性强，散步时适合固定路线和温和社交。", sizeFactor: 1.05 },
  beagle: { name: "比格", weightRange: [9, 13], walk: [60, 90], calories: [220, 330], speed: "3.5-6 km/h", tip: "注意嗅闻和饮食控制", intro: "比格爱嗅闻也容易贪吃，散步能帮助消耗精力和控制体重。", sizeFactor: 1.05 },
  schnauzer: { name: "雪纳瑞", weightRange: [6, 10], walk: [45, 70], calories: [160, 250], speed: "3-5.5 km/h", tip: "关注皮肤和体重", intro: "雪纳瑞活泼警觉，适合中等强度的规律散步。", sizeFactor: 1.1 },
  samoyed: { name: "萨摩耶", weightRange: [17, 30], walk: [70, 100], calories: [320, 500], speed: "3.5-6.5 km/h", tip: "夏天注意降温", intro: "萨摩耶毛量大，运动需求不低，高温天气要减少强度。", sizeFactor: 0.9 },
  frenchie: { name: "法斗", weightRange: [8, 14], walk: [25, 45], calories: [130, 220], speed: "2-4 km/h", tip: "避免高温和剧烈运动", intro: "法斗短鼻，耐热和耐力较弱，更适合短时多次轻松散步。", sizeFactor: 1 },
  mixed: { name: "混血犬", weightRange: [8, 25], walk: [45, 80], calories: [180, 380], speed: "3-6 km/h", tip: "按体型和状态动态调整", intro: "混血犬差异较大，可以先按体型和当天状态调整运动量。", sizeFactor: 1 },
};

let dogs = [
  { id: 1, name: "Luna", owner: "小林", breed: "golden", age: 3, weight: 30, neutered: false, caloriesNow: 128, walkMinutes: 45, distanceKm: 3.2 },
  { id: 2, name: "Momo", owner: "小林", breed: "poodle", age: 5, weight: 5, neutered: true, caloriesNow: 64, walkMinutes: 28, distanceKm: 1.4 },
];

let activeDogId = 1;
const state = { mode: "leash" };
const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => Array.from(document.querySelectorAll(selector));

const walkRecords = [
  ["今天", "45 min", "3.2 km", "128 kcal", "达成 60%"],
  ["昨天", "72 min", "5.1 km", "326 kcal", "达成 94%"],
  ["周一", "58 min", "4.0 km", "242 kcal", "达成 76%"],
  ["周日", "81 min", "5.8 km", "382 kcal", "达成 108%"],
  ["周六", "36 min", "2.4 km", "118 kcal", "达成 48%"],
  ["周五", "64 min", "4.6 km", "287 kcal", "达成 82%"],
  ["周四", "52 min", "3.8 km", "226 kcal", "达成 70%"],
];

const poopRecords = [
  ["今天", "黄褐色", "成型", "8.6"],
  ["昨天", "黄褐色", "偏软", "7.2"],
  ["周一", "浅褐色", "成型", "8.8"],
  ["周日", "深褐色", "偏软", "6.9"],
  ["周六", "黄褐色", "成型", "8.5"],
  ["周五", "黄褐色", "颗粒偏干", "7.4"],
  ["周四", "黄褐色", "成型", "8.7"],
];

function activeDog() {
  return dogs.find((dog) => dog.id === activeDogId) || dogs[0];
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function midpoint(range) {
  return (range[0] + range[1]) / 2;
}

function getAgeFactor(age) {
  if (age < 1) return 0.6;
  if (age > 7) return 0.7;
  return 1;
}

function getWeightFactor(weight, profile) {
  const ideal = midpoint(profile.weightRange);
  if (weight < ideal * 0.85) return 1.1;
  if (weight > ideal * 1.15) return 0.8;
  return 1;
}

function getRecommendation(dog) {
  const profile = breedProfiles[dog.breed] || breedProfiles.mixed;
  const neuteredFactor = dog.neutered ? 0.85 : 1;
  const factor = getAgeFactor(Number(dog.age)) * getWeightFactor(Number(dog.weight), profile) * neuteredFactor;
  return {
    walkTarget: Math.round(midpoint(profile.walk) * factor),
    calorieTarget: Math.round(midpoint(profile.calories) * factor),
    profile,
  };
}

function breedOptions(selected) {
  return Object.entries(breedProfiles)
    .map(([key, profile]) => `<option value="${key}" ${selected === key ? "selected" : ""}>${profile.name}</option>`)
    .join("");
}

function renderDogProfiles() {
  $("#dogCount").textContent = `${dogs.length} 只狗狗`;
  $("#activeDogHint").textContent = `当前查看 ${activeDog().name}`;
  $("#dogList").innerHTML = dogs
    .map((dog) => {
      const { walkTarget, calorieTarget, profile } = getRecommendation(dog);
      return `
        <article class="dog-profile-card ${dog.id === activeDogId ? "active" : ""}" data-dog-id="${dog.id}">
          <button class="dog-profile-avatar" type="button" data-select-dog="${dog.id}" aria-label="切换到 ${dog.name}">${dog.name.slice(0, 1).toUpperCase()}</button>
          <div>
            <div class="profile-card-grid">
              <label>名字<input data-field="name" value="${dog.name}" /></label>
              <label>品种<select data-field="breed">${breedOptions(dog.breed)}</select></label>
              <label>年龄<input data-field="age" type="number" min="0.3" max="18" step="0.5" value="${dog.age}" /></label>
              <label>体重 kg<input data-field="weight" type="number" min="2" max="60" step="0.5" value="${dog.weight}" /></label>
              <label class="toggle-row"><input data-field="neutered" type="checkbox" ${dog.neutered ? "checked" : ""} /><span>已绝育</span></label>
            </div>
            <div class="dog-profile-footer">
              <p class="breed-intro">${profile.intro}</p>
              <span class="walk-recommend">建议 ${walkTarget} min · ${calorieTarget} kcal</span>
            </div>
          </div>
          <div class="dog-card-actions">
            <button class="select-dog-button" type="button" data-select-dog="${dog.id}">查看</button>
            <button class="trash-button" type="button" data-delete-dog="${dog.id}" ${dogs.length <= 1 ? "disabled" : ""} aria-label="删除 ${dog.name}">
              <svg><use href="#icon-trash"></use></svg>
            </button>
          </div>
        </article>
      `;
    })
    .join("");

  $$("[data-select-dog]").forEach((button) => button.addEventListener("click", () => selectDog(Number(button.dataset.selectDog))));
  $$("[data-delete-dog]").forEach((button) => button.addEventListener("click", () => deleteDog(Number(button.dataset.deleteDog))));
  $$(".dog-profile-card [data-field]").forEach((field) => {
    field.addEventListener("input", () => updateDogField(Number(field.closest(".dog-profile-card").dataset.dogId), field));
  });
}

function selectDog(id) {
  activeDogId = id;
  renderAll();
}

function updateDogField(id, field) {
  const dog = dogs.find((item) => item.id === id);
  if (!dog) return;
  const key = field.dataset.field;
  if (key === "neutered") dog.neutered = field.checked;
  else if (key === "age" || key === "weight") dog[key] = Number(field.value || 0);
  else dog[key] = field.value.trim() || (key === "name" ? "新朋友" : "主人");
  activeDogId = id;
  renderAll();
}

function renderDogInfo() {
  const dog = activeDog();
  const { walkTarget, calorieTarget, profile } = getRecommendation(dog);
  const progress = clamp(Math.round((dog.caloriesNow / calorieTarget) * 100), 0, 100);
  const walkProgress = clamp(Math.round((dog.walkMinutes / walkTarget) * 100), 0, 140);
  const score = clamp(94 - Math.max(0, progress - 100) / 2 + (progress > 45 ? 3 : 0), 72, 98);

  $("#todayTitle").textContent = `${dog.name} 今天很好`;
  $("#walkTitle").textContent = `陪 ${dog.name} 走一圈`;
  $("#mapDogName").textContent = dog.name;
  $("#safetyScore").textContent = Math.round(score);
  $("#caloriesNow").textContent = dog.caloriesNow;
  $("#walkCalories").textContent = dog.caloriesNow;
  $("#calorieTarget").textContent = calorieTarget;
  $("#calorieProgressBar").style.width = `${progress}%`;
  $("#walkTarget").textContent = walkTarget;
  $("#recommendCalories").textContent = calorieTarget;
  $("#speedTarget").textContent = profile.speed;
  $("#walkPercent").textContent = `${walkProgress}%`;
  $("#careTip").textContent = profile.tip;
  $("#walkMinutes").textContent = dog.walkMinutes;
  $("#walkDistance").textContent = dog.distanceKm.toFixed(1);
  $("#assistantDogName").textContent = `${dog.name} · ${profile.name}`;
  $("#assistantTarget").textContent = `${walkTarget} min · ${calorieTarget} kcal`;
}

function activityCoefficient(speed) {
  if (speed <= 3) return 2;
  if (speed <= 6) return 3;
  if (speed <= 10) return 4.5;
  return 6;
}

function estimateMinuteCalories(speed) {
  const dog = activeDog();
  const profile = breedProfiles[dog.breed] || breedProfiles.mixed;
  const rer = 70 * Math.pow(Number(dog.weight), 0.75);
  return (rer / 1440) * activityCoefficient(speed) * profile.sizeFactor;
}

function updateVitals(randomize = false) {
  const dog = activeDog();
  const speed = randomize ? (2.2 + Math.random() * 7.8).toFixed(1) : $("#speedNow").textContent;
  const heart = randomize ? Math.round(86 + Number(speed) * 8 + Math.random() * 18) : Number($("#heartRateNow").textContent);
  const temp = randomize ? (38.1 + Math.random() * 1.1).toFixed(1) : $("#tempNow").textContent;
  const distance = state.mode === "free" ? (2 + Math.random() * 11).toFixed(1) : (1.2 + Math.random() * 3.4).toFixed(1);
  const tension = state.mode === "leash" ? (5 + Math.random() * 19).toFixed(1) : "0.0";
  const minuteCalories = estimateMinuteCalories(Number(speed));

  dog.caloriesNow = Math.round(dog.caloriesNow + minuteCalories * 3);
  dog.walkMinutes = clamp(dog.walkMinutes + 2, 0, 160);
  dog.distanceKm = Number((dog.distanceKm + Number(speed) / 90).toFixed(1));

  $("#speedNow").textContent = speed;
  $("#heartRateNow").textContent = heart;
  $("#tempNow").textContent = temp;
  $("#distanceNow").textContent = distance;
  $("#tensionNow").textContent = tension;
  $("#avgHeart").textContent = Math.round((heart + 112) / 2);
  $("#paceLabel").textContent = Number(speed) > 6 ? "奔跑" : Number(speed) > 3 ? "快走" : "慢走";
  $("#heartLabel").textContent = heart > 160 ? "偏高" : "正常";
  $("#tempLabel").textContent = Number(temp) > 39.2 ? "偏高" : "正常";
  $("#distanceLabel").textContent = Number(distance) > 10 ? "偏远" : "安全";
  $("#fenceHint").textContent = Number(distance) > 10 ? "接近边界" : "安全";

  const riskyTension = Number(tension) > 15;
  $("#tensionHint").textContent = riskyTension ? "偏紧" : "正常";
  $("#lockState").textContent = riskyTension ? "提醒中" : "未锁定";
  updateHealthPill(heart, Number(temp), Number(distance), riskyTension);
  drawHeartLine(heart);
  drawSpeedLine(Number(speed));
  renderAll();
}

function updateHealthPill(heart, temp, distance, riskyTension) {
  const pill = $("#healthPill");
  pill.classList.remove("good", "warn", "danger");
  if (heart > 175 || temp > 39.5 || distance > 15) {
    pill.textContent = "关注";
    pill.classList.add("danger");
    $("#todayAlertCount").textContent = "3 条";
    return;
  }
  if (riskyTension || heart > 155 || temp > 39.1 || distance > 10) {
    pill.textContent = "提醒";
    pill.classList.add("warn");
    $("#todayAlertCount").textContent = "2 条";
    return;
  }
  pill.textContent = "健康";
  pill.classList.add("good");
  $("#todayAlertCount").textContent = "1 条";
}

function drawHeartLine(currentHeart) {
  const values = Array.from({ length: 9 }, (_, index) => Math.round(clamp(currentHeart - 20 + Math.sin(index) * 14 + Math.random() * 18, 80, 180)));
  const points = values.map((value, index) => `${Math.round((420 / 8) * index)},${Math.round(170 - ((value - 70) / 120) * 135)}`);
  $("#heartLine").setAttribute("points", points.join(" "));
  $("#heartRange").textContent = `${Math.min(...values)}-${Math.max(...values)} bpm`;
}

function drawSpeedLine(currentSpeed) {
  const values = Array.from({ length: 9 }, (_, index) => Number(clamp(currentSpeed - 1.6 + Math.sin(index * 1.25) * 1.5 + Math.random() * 2.2, 1.2, 10.8).toFixed(1)));
  const points = values.map((value, index) => `${Math.round((420 / 8) * index)},${Math.round(170 - ((value - 1) / 10) * 135)}`);
  $("#speedLine").setAttribute("points", points.join(" "));
  $("#speedRange").textContent = `${Math.min(...values).toFixed(1)}-${Math.max(...values).toFixed(1)} km/h`;
}

function setMode(mode) {
  state.mode = mode;
  $$(".mode-button").forEach((button) => button.classList.toggle("active", button.dataset.mode === mode));
  $("#lockState").textContent = mode === "free" ? "未启用" : "未锁定";
  $("#tensionHint").textContent = mode === "free" ? "无牵引" : "正常";
  updateVitals(true);
}

function setView(viewId) {
  $$(".view").forEach((view) => view.classList.toggle("active", view.id === viewId));
  $$(".nav-item").forEach((button) => button.classList.toggle("active", button.dataset.view === viewId));
  window.scrollTo({ top: 0, behavior: "smooth" });
}

function renderRecords() {
  $("#walkRecordList").innerHTML = walkRecords
    .map(([day, time, distance, calories, status]) => `<div class="record-item"><strong>${day}</strong><div><strong>${time} · ${distance}</strong><p>${calories} · ${status}</p></div><span class="record-score">散步</span></div>`)
    .join("");
  $("#poopRecordList").innerHTML = poopRecords
    .map(([day, color, shape, score]) => `<div class="record-item"><strong>${day}</strong><div><strong>${color} · ${shape}</strong><p>健康评分 ${score}</p></div><span class="record-score">${score}</span></div>`)
    .join("");
}

function addDog() {
  const nextId = Math.max(...dogs.map((dog) => dog.id)) + 1;
  dogs.push({ id: nextId, name: `新朋友 ${dogs.length + 1}`, owner: activeDog().owner, breed: "corgi", age: 2, weight: 11, neutered: false, caloriesNow: 0, walkMinutes: 0, distanceKm: 0 });
  activeDogId = nextId;
  renderAll();
  setView("profile");
}

function deleteDog(id) {
  if (dogs.length <= 1) return;
  dogs = dogs.filter((dog) => dog.id !== id);
  if (activeDogId === id) activeDogId = dogs[0].id;
  renderAll();
}

function renderAll() {
  renderDogInfo();
  renderDogProfiles();
  renderRecords();
}

function appendMessage(role, text) {
  const item = document.createElement("div");
  item.className = `chat-message ${role}`;
  item.textContent = text;
  $("#chatBox").appendChild(item);
  $("#chatBox").scrollTop = $("#chatBox").scrollHeight;
}

function currentMetrics() {
  return {
    speed: $("#speedNow").textContent,
    heartRate: $("#heartRateNow").textContent,
    temperature: $("#tempNow").textContent,
    distance: $("#distanceNow").textContent,
    tension: $("#tensionNow").textContent,
    pace: $("#paceLabel").textContent,
  };
}

function localAssistantReply(question) {
  const dog = activeDog();
  const { walkTarget, calorieTarget, profile } = getRecommendation(dog);
  const remainingWalk = Math.max(0, walkTarget - dog.walkMinutes);
  const remainingCalories = Math.max(0, calorieTarget - dog.caloriesNow);
  const metrics = currentMetrics();
  const lower = question.toLowerCase();
  const parts = [
    `${dog.name} 是${profile.name}，${profile.intro}`,
    `今天目标约 ${walkTarget} 分钟 / ${calorieTarget} kcal，目前已完成 ${dog.walkMinutes} 分钟 / ${dog.caloriesNow} kcal。`,
  ];

  if (question.includes("多久") || question.includes("散步") || question.includes("运动")) {
    parts.push(remainingWalk > 0 ? `建议再安排约 ${remainingWalk} 分钟轻中等强度散步，剩余热量约 ${remainingCalories} kcal。` : "今天运动目标基本完成，后面以轻松放松和补水为主。");
  } else if (question.includes("便便") || question.includes("软便") || question.includes("拉稀")) {
    parts.push("便便偏软时先观察饮食变化、零食和饮水，今天可以降低运动强度；如果连续 24-48 小时异常、带血或精神变差，建议联系兽医。");
  } else if (question.includes("心率") || lower.includes("heart")) {
    parts.push(`当前心率约 ${metrics.heartRate} bpm。如果持续偏高，先降速、休息和补水，等呼吸平稳后再继续。`);
  } else if (question.includes("体温") || question.includes("热")) {
    parts.push(`当前体温约 ${metrics.temperature} °C。高温天气建议缩短散步、避开暴晒，并观察喘气和精神状态。`);
  } else {
    parts.push(`结合当前速度 ${metrics.speed} km/h、状态 ${metrics.pace}，建议以舒适稳定为主，不要突然加大强度。`);
  }
  return parts.join("\n");
}

async function askAssistant(question) {
  appendMessage("user", question);
  appendMessage("assistant", "正在思考...");
  const pending = $("#chatBox").lastElementChild;
  try {
    const dog = activeDog();
    const response = await fetch("/api/assistant", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ question, dog, recommendation: getRecommendation(dog), metrics: currentMetrics() }),
    });
    if (!response.ok) throw new Error("assistant unavailable");
    const data = await response.json();
    pending.textContent = data.answer || localAssistantReply(question);
    $("#assistantStatus").textContent = data.mode === "deepseek" ? "DeepSeek 已连接" : "本地建议";
  } catch {
    pending.textContent = localAssistantReply(question);
    $("#assistantStatus").textContent = "本地建议";
  }
}

async function checkAssistantStatus() {
  try {
    const response = await fetch("/api/assistant/status");
    const data = await response.json();
    $("#assistantStatus").textContent = data.deepseek ? "DeepSeek 已连接" : "本地建议";
  } catch {
    $("#assistantStatus").textContent = "本地建议";
  }
}

function bindEvents() {
  $$(".nav-item").forEach((button) => button.addEventListener("click", () => setView(button.dataset.view)));
  $$("[data-go]").forEach((button) => button.addEventListener("click", () => setView(button.dataset.go)));
  $("#simulateBtn").addEventListener("click", () => updateVitals(true));
  $$(".mode-button").forEach((button) => button.addEventListener("click", () => setMode(button.dataset.mode)));
  $("#addDogBtn").addEventListener("click", addDog);
  $("#chatForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const input = $("#chatInput");
    const question = input.value.trim();
    if (!question) return;
    input.value = "";
    askAssistant(question);
  });
  $$("[data-question]").forEach((button) => {
    button.addEventListener("click", () => askAssistant(button.dataset.question));
  });
}

bindEvents();
renderAll();
updateVitals(false);
checkAssistantStatus();
