# Проверка обновления тиммейта и совместимости

**Историческая проверка `80c752b`. Блокер устранён коммитом `b461826`: исходники и шейдеры получены и объединены локально. Актуальные результаты: [STAGE1_INTEGRATED_VALIDATION.md](STAGE1_INTEGRATED_VALIDATION.md). Текст ниже сохраняет результаты предыдущей проверки, а не текущее состояние.**

Проверен удалённый `main`: **80c752b07a5c52ab00701bda20f3d710c8697602**, сообщение `Stage 1: terrain, player and camera`. На момент проверки на origin есть только ветка `main`, теги отсутствуют. Приложенный пользователем README побайтно совпадает с README этого коммита. Остальные три присланных документа прочитаны и сохранены без изменений в `docs/teammate-stage1/`.

## Блокер: исходники не попали в push

В удалённом коммите всего **6 файлов**:

```text
.gitignore
Dependencies.props
EndlessRunner.sln
EndlessRunner.vcxproj
EndlessRunner.vcxproj.user
README.md
```

`EndlessRunner.vcxproj` ссылается на отсутствующие входные файлы. Для чистой копии `main` нет ни исходников приложения, ни шейдеров. README описывает реализацию, но проверить её работу по этому коммиту невозможно. Возможные файлы в корне тоже проверены — их нет. Внешние библиотеки GLAD/FreeGLUT/GLM могут устанавливаться отдельно; их отсутствие в Git само по себе не ошибка.

Нужно добавить исходники тиммейта:

```text
src/main.cpp
src/HeightField.cpp
src/HeightField.h
src/Player.cpp
src/Player.h
src/Renderer.cpp
src/Renderer.h
src/Shader.cpp
src/Shader.h
src/Camera.h
src/GlDiagnostics.h
src/Keyboard.h
shaders/terrain.vert
shaders/player.vert
shaders/lit.frag
shaders/hud.vert
shaders/hud.frag
```

В Git также отсутствуют `START_HERE_RU.md`, `SHADER_NOTES_RU.md`, `INTEGRATION_DANIYAR.md`, на которые ссылаются README и проект. Их копии уже доступны здесь из вложений пользователя.

## Конфликты и различия

| Область | Локальный прототип Данияра | Документы/конфигурация тиммейта | Решение при интеграции |
|---|---|---|---|
| Окно и функции OpenGL | GLFW + GLEW | FreeGLUT + GLAD 1 | Сохранить существующее окно FreeGLUT и загрузчик GLAD в общей сцене; адаптировать obstacle renderer. Не подключать два loader header в одном translation unit. |
| Загрузка шейдеров | Класс `runner::Shader` | Функция `loadProgram` из `Shader.cpp` | Подключить препятствия к общему Shader API после чтения исходников. Не заменять `Shader.*` целиком. |
| Скорость | 12 м/с | 8 м/с | Один общий источник скорости; стартовое значение 8 м/с по Stage 1, пока без роста скорости. |
| Период поверхности | 256 м | 128 м | Использовать период настоящего HeightField; не переносить константу 256 в интегрированную сцену. |
| Семплирование | Предложенный ранее `phase - z` | `v = (z - phase) / terrainPeriod`; `surfaceHeight(x,z,phase,displaced)` | Вызывать `surfaceHeight` напрямую. Не инвертировать Z повторно и не копировать старый пример `sampleHeight`. |
| Контакт с землёй | Временная функция возвращает 0 | Интерполяция по треугольникам видимой сетки | Позиция центра препятствия: `surfaceHeight(...) + height/2`; учитывать T. |
| Игровой цикл | Свой `Game`, Enter/ready, P/R | Свой update, player physics, P/R | Один dt, одна фаза, общие pause/reset; препятствия не должны обновляться дважды. |
| Клавиша C | Сброс камеры | Follow/orbit | Сохранить C из Stage 1, добавить I для instancing. |
| Геометрия игрока | Статичный маркер | 0.8×1.2×0.8, feetY — низ; X в [-4.2,4.2] | Удалить маркер при подключении настоящего Player. |
| Visual Studio | Локальная сборка через CMake | Список source-файлов в `.vcxproj` | Добавить фактически интегрированные obstacle/game-файлы; не оставлять два main. |

Сейчас пересекаются пути изменений **README.md и .gitignore**. После загрузки исходников ожидаются совпадения `src/main.cpp`, `src/Shader.cpp`, `src/Shader.h`, `shaders/hud.vert`, `shaders/hud.frag`. Одинаковые имена не гарантируют одинаковые интерфейсы. Это потенциальные конфликты интеграции; незавершённого Git merge в рабочей папке нет.

## Что проверено и сохранено

- Выполнен `git fetch`, исходная локальная ветка не перемещена на неполный remote commit.
- Полная копия текущих исходников и документов Данияра сохранена в `/workspace/review-backups/`, рядом с проверенным Git bundle.
- `.gitignore` дополнен правилами Windows/Visual Studio из новой версии без потери правил CMake и локальных артефактов.
- Инструкция `docs/INTEGRATION.md` исправлена под контракт `surfaceHeight` из предоставленных документов; изменения рабочего runtime отложены до появления кода тиммейта.
- Локальная CMake-сборка прошла; CTest: 1 тест, 212 проверок; OpenGL smoke: 1 vs 96 draw calls, совпадение изображений, движение, свет, resize, отсутствие GL ошибок.
- Windows-сборка Stage 1, его height map, прыжок, клавиатура и совместная сцена **не проверены**: необходимых исходников нет.

## Что требуется для продолжения

Тиммейту нужно отправить отсутствующие `src/`, `shaders/` и три MD-файла из своей рабочей папки. Сначала следует проверить `git status` и список подготовленных к коммиту файлов. Пример команд, выполняемых **в его папке проекта**:

```sh
git add src shaders START_HERE_RU.md SHADER_NOTES_RU.md INTEGRATION_DANIYAR.md
git diff --cached --stat
git status --short
git commit -m "Add Stage 1 source, shaders and documentation"
git push origin main
```

Команды предназначены для папки тиммейта с настоящими HeightField/Player/Renderer. Запускать их здесь вместо него нельзя: здесь другой прототип. DLL, bin, obj, .vs и пользовательские настройки коммитить не нужно. Файл `EndlessRunner.vcxproj.user` уже отслеживается в main, хотя `*.user` игнорируется; в текущем коммите он пустой и сборку не блокирует.

После полного push можно выполнить настоящее объединение: сохранить terrain/player/camera, перенести obstacle renderer на общий GLAD/Shader API, подключить его в Renderer перед HUD и проверить единые pause/reset, height-map toggle и instancing ON/OFF. Столкновения/game-over и рост скорости в присланных документах обозначены последующими этапами; они не заявляются как выполненные в этой проверке.
