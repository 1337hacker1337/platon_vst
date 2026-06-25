# PLATON V1 — distorted 808 bass (VST3 / AU)

Моно-синтезатор баса под жирные дисторшн-808 в духе che / yrsci.
Синус с питч-envelope «knock», глайд между нотами, пред-дисторшн HPF
(режешь низ → грязь ест только верх), 6 алгоритмов дисторшена,
параллельный чистый саб и tone-фильтр.

Звук DSP полностью совпадает с браузерной версией `PlatonV1.html` —
сначала накрути там вайб, потом собери плагин и повтори настройки.

---

## ★ Самый простой способ — собрать в облаке (ничего не ставить на комп)

GitHub сам скомпилит готовый `.vst3` под Windows, ты только скачаешь файл.

1. Заведи бесплатный аккаунт на **github.com**.
2. Жми **New repository** →любое имя (напр. `platon-vst`) → **Create repository**.
3. На странице репо: **Add file → Upload files**. Перетащи туда ВСЁ содержимое
   этой папки (файл `CMakeLists.txt`, папку `Source`, папку `.github`, `README.md`).
   Важно: тащи именно файлы/папки, чтобы сохранилась структура. Жми **Commit changes**.
4. Открой вкладку **Actions** — сборка запустится сама (идёт ~10–15 минут,
   первый раз дольше). Дождись зелёной галочки.
5. Кликни на завершённый запуск → внизу в **Artifacts** скачай
   **PlatonV1-Windows-VST3** (это zip). Распакуй.
6. Скопируй `Platon V1.vst3` в `C:\Program Files\Common Files\VST3\`.
7. В FL Studio: **Options → Manage plugins → Find installed plugins** (rescan),
   найди **Platon V1** и кидай в Channel Rack.

Маководам там же лежит артефакт **PlatonV1-macOS** (VST3 + AU + Standalone).

Если что-то не собралось — открой красный запуск в Actions, там видно лог ошибки.

---

## Локально на своём компе (если хочешь без GitHub)

### Что нужно

**Windows**
1. Поставь **Visual Studio 2022 Community** (бесплатно), при установке отметь
   галку **"Разработка классических приложений на C++" / "Desktop development with C++"**.
2. Поставь **CMake** → https://cmake.org/download/ (при установке выбери "Add to PATH").
3. Открой папку проекта в командной строке (PowerShell / cmd) и выполни:
   ```
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release --target PlatonV1_VST3
   ```
   Первая сборка сама скачает JUCE с интернета и займёт ~10–20 минут.
4. Готовый плагин: `build\PlatonV1_artefacts\Release\VST3\Platon V1.vst3`.
   Он автоматически копируется в `C:\Program Files\Common Files\VST3\`
   (если не скопировался из-за прав — закинь туда вручную).

**macOS**
1. Поставь **Xcode** + Command Line Tools (`xcode-select --install`) и **CMake**.
2. В папке проекта:
   ```
   cmake -B build -G Xcode
   cmake --build build --config Release
   ```
3. VST3 → `~/Library/Audio/Plug-Ins/VST3/`, AU → `~/Library/Audio/Plug-Ins/Components/`
   (копируются автоматически).

---

## Как завести в FL Studio

1. **Options → Manage plugins**.
2. Если ставил VST3 не в стандартную папку — добавь путь в **Plugin search paths**.
3. Жми **Find installed plugins** (Start scan).
4. Открой браузер плагинов, найди **Platon V1**, кинь в Channel Rack как инструмент.
5. Рисуй ноты в пиано-ролле. В моно-режиме ноты глайдят друг в друга
   (длина глайда — ручка **GLIDE**). Под 808 ставь короткие ноты с релизом.

---

## Шпаргалка по звуку

| Хочешь | Крути |
|---|---|
| Базовый knock 808 | **P.ENV** 12–24 st, **P.TIME** 40–70 ms |
| Жир/овердрайв (che) | **DIST = Tanh**, **DRIVE** 8–12, **DIST MIX** ~0.9 |
| Злая грязь (yrsci/rage) | **DIST = Foldback / Hard Clip**, **DRIVE** 10–20 |
| Бас читается, но низ не пропадает | подними **PRE HPF** до 150–300 Гц + добавь **SUB** |
| Приручить резкость сверху | опусти **TONE** (cutoff LPF) |
| Скрежет/лоу-фай | **DIST = Bitcrush**, играй **DRIVE** |
| Глайды как в plugg | моно по умолчанию, **GLIDE** 40–90 ms |

**Логика сигнала:** osc → (pre-HPF → drive → distortion) смешивается с сухим телом
через DIST MIX, параллельно добавляется чистый SUB → TONE LPF → amp envelope → выход
с защитным soft-clip.

Стартовые пресеты (CHE / YRSCI / RAGE / CRUNCH / CLEAN) есть в браузерной версии —
просто перенеси значения ручек в плагин.

---

## Структура проекта
```
PlatonV1-VST/
├── CMakeLists.txt          # тянет JUCE сам, собирает VST3 + Standalone (+AU на Mac)
└── Source/
    ├── PluginProcessor.h/.cpp   # звуковой движок и параметры
    └── PluginEditor.h/.cpp      # интерфейс (крутилки)
```

Менеджер-код, имя компании и т.д. поменяй в `CMakeLists.txt`
(`COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE`).

---

## v1 — полный движок (синхронизирован с браузерной версией)

Плагин теперь повторяет браузерный Platon V1 один-в-один. Параметры:
OSC: tune, fine, shape (sine→tri→saw→square), glide, unison, detune.
OSC2 / FM: уровень OSC2, интервал (semi), FM-глубина, FM-envelope, noise.
Pitch knock, ADSR + punch + click, дисторшн (6 типов) + bias + crush + focus,
pre-HPF, резонансный фильтр (LP/BP/HP) с огибающей, чистый mono sub (±окт),
LFO (pitch/filter/volume/drive), chorus, glue-компрессор, width, выходной clip.

Пресеты и состояние сохраняются вместе с проектом FL Studio автоматически
(плагин сериализует все параметры). Свои пресеты можно гонять в браузерной
версии и переносить значения, либо сохранять как пресеты FL поверх плагина.

### Про звук в файл
Браузерная версия умеет «bounce 808 → RENDER WAV»: выбираешь ноту и качаешь
WAV-ваншот, который тащишь в FL Studio. В самом плагине бонс делается стандартно
средствами FL (рендер паттерна/трека в WAV или MP3 через File → Export).
