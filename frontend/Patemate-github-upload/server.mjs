import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { extname, join, normalize } from "node:path";

const root = process.cwd();

async function loadLocalEnv() {
  try {
    const text = await readFile(join(root, ".env.local"), "utf8");
    for (const line of text.split(/\r?\n/)) {
      const trimmed = line.trim();
      if (!trimmed || trimmed.startsWith("#") || !trimmed.includes("=")) continue;
      const [key, ...valueParts] = trimmed.split("=");
      if (!process.env[key]) process.env[key] = valueParts.join("=").replace(/^["']|["']$/g, "");
    }
  } catch {
    // .env.local is optional.
  }
}

await loadLocalEnv();

const port = Number(process.env.PORT || 4173);

const contentTypes = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".png": "image/png",
  ".svg": "image/svg+xml",
};

function sendJson(res, status, data) {
  res.writeHead(status, { "Content-Type": "application/json; charset=utf-8" });
  res.end(JSON.stringify(data));
}

async function readBody(req) {
  const chunks = [];
  for await (const chunk of req) chunks.push(chunk);
  return Buffer.concat(chunks).toString("utf8");
}

function fallbackAnswer({ question, dog, recommendation }) {
  const breedName = recommendation?.profile?.name || "狗狗";
  const intro = recommendation?.profile?.intro || "";
  const walkTarget = recommendation?.walkTarget || 45;
  const calorieTarget = recommendation?.calorieTarget || 180;
  const doneWalk = Number(dog?.walkMinutes || 0);
  const doneCalories = Number(dog?.caloriesNow || 0);
  const remainingWalk = Math.max(0, walkTarget - doneWalk);
  const remainingCalories = Math.max(0, calorieTarget - doneCalories);
  return [
    `${dog?.name || "狗狗"} 是${breedName}。${intro}`,
    `今日目标约 ${walkTarget} 分钟 / ${calorieTarget} kcal，目前已完成 ${doneWalk} 分钟 / ${doneCalories} kcal。`,
    remainingWalk > 0 ? `还可以安排约 ${remainingWalk} 分钟轻中等强度散步，剩余约 ${remainingCalories} kcal。` : "今天运动目标基本完成，后续以轻松活动和补水为主。",
    `关于“${question}”，如果出现持续异常、精神变差、便血、呕吐或呼吸困难，请及时咨询兽医。`,
  ].join("\n");
}

function asksConnectionStatus(question) {
  return /deepseek|大模型|api|接口|连接|连上|连通/i.test(String(question || ""));
}

async function handleAssistant(req, res) {
  const body = JSON.parse(await readBody(req) || "{}");
  const apiKey = process.env.DEEPSEEK_API_KEY;

  if (!apiKey) {
    sendJson(res, 200, { answer: fallbackAnswer(body), mode: "local-fallback" });
    return;
  }

  const dog = body.dog || {};
  const recommendation = body.recommendation || {};
  const metrics = body.metrics || {};
  const remainingWalk = Math.max(0, Number(recommendation.walkTarget || 0) - Number(dog.walkMinutes || 0));
  const remainingCalories = Math.max(0, Number(recommendation.calorieTarget || 0) - Number(dog.caloriesNow || 0));
  const prompt = [
    "你是一个宠物健康助手，回答要简洁、温和、面向普通养宠用户。",
    "你必须体现你看到了当前狗狗档案和实时数据，不要泛泛而谈。",
    "优先给出明确结论，然后给2-4条行动建议。",
    "不能替代兽医诊断；遇到持续异常、精神差、呕吐、便血、呼吸困难等情况要建议就医。",
    `当前狗狗：${dog.name || "未命名"}，品种 ${recommendation.profile?.name || dog.breed || "未知"}，年龄 ${dog.age || "未知"} 岁，体重 ${dog.weight || "未知"} kg。`,
    `品种特点：${recommendation.profile?.intro || "暂无"}`,
    `今日建议：散步 ${recommendation.walkTarget || "未知"} 分钟，热量 ${recommendation.calorieTarget || "未知"} kcal；已完成 ${dog.walkMinutes || 0} 分钟，${dog.caloriesNow || 0} kcal；剩余约 ${remainingWalk} 分钟，${remainingCalories} kcal。`,
    `当前实时数据：速度 ${metrics.speed || "未知"} km/h，心率 ${metrics.heartRate || "未知"} bpm，体温 ${metrics.temperature || "未知"} °C，人狗距离 ${metrics.distance || "未知"} m，牵引力度 ${metrics.tension || "未知"} N，运动状态 ${metrics.pace || "未知"}。`,
  ].join("\n");

  const response = await fetch("https://api.deepseek.com/chat/completions", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Bearer ${apiKey}`,
    },
    body: JSON.stringify({
      model: "deepseek-chat",
      messages: [
        { role: "system", content: prompt },
        { role: "user", content: String(body.question || "") },
      ],
      temperature: 0.4,
      max_tokens: 500,
    }),
  });

  if (!response.ok) {
    sendJson(res, 200, { answer: fallbackAnswer(body), mode: "api-fallback" });
    return;
  }

  const data = await response.json();
  if (asksConnectionStatus(body.question)) {
    sendJson(res, 200, { answer: "后端已经成功连接 DeepSeek，大模型接口可用。", mode: "deepseek", deepseek: true });
    return;
  }
  sendJson(res, 200, { answer: data.choices?.[0]?.message?.content || fallbackAnswer(body), mode: "deepseek", deepseek: true });
}

async function handleStatic(req, res) {
  const url = new URL(req.url, `http://${req.headers.host}`);
  const pathname = url.pathname === "/" ? "/index.html" : decodeURIComponent(url.pathname);
  const safePath = normalize(pathname).replace(/^(\.\.[/\\])+/, "");
  const filePath = join(root, safePath);
  const ext = extname(filePath);
  const content = await readFile(filePath);
  res.writeHead(200, { "Content-Type": contentTypes[ext] || "application/octet-stream" });
  res.end(content);
}

createServer(async (req, res) => {
  try {
    if (req.method === "GET" && req.url === "/api/assistant/status") {
      sendJson(res, 200, { deepseek: Boolean(process.env.DEEPSEEK_API_KEY) });
      return;
    }
    if (req.method === "POST" && req.url === "/api/assistant") {
      await handleAssistant(req, res);
      return;
    }
    if (req.method === "GET" || req.method === "HEAD") {
      await handleStatic(req, res);
      return;
    }
    res.writeHead(405);
    res.end("Method Not Allowed");
  } catch (error) {
    if (req.url === "/api/assistant") {
      sendJson(res, 200, { answer: "助手暂时连接不上，我先建议你观察狗狗精神、食欲、体温和便便状态；如果异常持续，请及时咨询兽医。", mode: "error-fallback" });
      return;
    }
    res.writeHead(404);
    res.end("Not Found");
  }
}).listen(port, "127.0.0.1", () => {
  console.log(`PetPulse running at http://127.0.0.1:${port}/`);
});
