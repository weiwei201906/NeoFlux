# 甯歌闂

## 鍩虹

### NeoFlux 鏄粈涔堬紵

NeoFlux 鏄竴涓交閲忕骇璺ㄥ钩鍙?C++20 UI 妗嗘灦锛岄噰鐢ㄤ袱灞傛灦鏋勶細Application 灞傝繍琛屼笟鍔￠€昏緫鍜?Taitank flex 甯冨眬寮曟搸锛孯ender 灞傞€氳繃 tgfx锛堢Щ鍔ㄧ锛夋垨 GLFW锛堟闈㈢锛夋秷璐规覆鏌撳懡浠ゃ€?

### 涓轰粈涔堢敤 C++20锛?

C++20 鏀寔鍗忕▼锛坄co_await`锛夈€乧oncepts銆乣std::span`銆乨esignated initializers 鍜?`std::bit_ceil`銆傛鏋跺叏绋嬩娇鐢ㄨ繖浜涚壒鎬у疄鐜扮畝娲侀珮鏁堢殑浠ｇ爜銆?

### NeoFlux 鍙互鐢ㄤ簬鐢熶骇鐜鍚楋紵

NeoFlux 姝ｅ湪绉瀬寮€鍙戜腑銆傛牳蹇冩灦鏋勶紙widget 鏍戙€乫lex 甯冨眬銆佹覆鏌撶绾裤€佽緭鍏ュ垎鍙戯級宸插彲鐢ㄣ€傚綋鍓嶉檺鍒惰鏌ョ湅 GitHub issues銆?

## 鏋勫缓

### 濡備綍鏋勫缓 NeoFlux锛?

```bash
cmake -S . -B build
cmake --build build -j
```

绀轰緥鍜屾祴璇曢粯璁ゅ叧闂€傚惎鐢細

```bash
cmake -S . -B build -DNEOFLUX_BUILD_EXAMPLES=ON -DNEOFLUX_BUILD_TESTS=ON
```

### 涓轰粈涔堢涓€娆℃瀯寤哄緢鎱紵

NeoFlux 浣跨敤 FetchContent 涓嬭浇骞舵瀯寤虹涓夋柟渚濊禆锛坓log銆乬flags銆乬lfw銆乼aitank銆乫reetype銆乬test锛夈€傚悗缁瀯寤哄緢蹇紝鍥犱负渚濊禆缂撳瓨鍦?`thirdparty/_deps/` 涓€?

### 濡備綍浜ゅ弶缂栬瘧锛?

涓虹洰鏍囧钩鍙拌缃?CMake toolchain 鏂囦欢銆侼eoFlux 浣跨敤骞冲彴瀹忥紙`NEOFLUX_PLATFORM_DESKTOP`銆乣NEOFLUX_PLATFORM_MOBILE`銆乣NEOFLUX_PLATFORM_WINDOWS` 绛夛級閫夋嫨鍚堥€傜殑鍚庣銆?

## 娓叉煋

### 鏀寔鍝簺娓叉煋鍚庣锛?

- **Vulkan**锛堥粯璁わ級鈥?閫氳繃 tgfx
- **OpenGL** 鈥?閫氳繃 tgfx/GLFW
- **CPU** 鈥?杞欢娓叉煋锛堝洖閫€锛?

浣跨敤 `--render_backend=vulkan|gl|cpu` 閫夋嫨銆?

### 涓轰粈涔堢獥鍙ｆ槸榛戠殑锛?

甯歌鍘熷洜锛?
1. **鏈姞杞藉瓧浣?* 鈥?Text widget 闇€瑕佸瓧浣撶洰褰曚腑鏈?`.ttf`/`.otf` 鏂囦欢銆傚湪 `Init()` 鍓嶈皟鐢?`app.SetFontDir("./fonts/")`銆?
2. **鏈?push route** 鈥?鎵€鏈夌ず渚嬮渶瑕?`RouteRegistry::RegisterRoute()` 鐒跺悗 `app.PushRoute("/")`銆?
3. **绐楀彛鏈毚闇?* 鈥?鏌愪簺骞冲彴涓婄涓€甯у彲鑳介渶瑕?resize 鎴?focus 浜嬩欢銆?

### 涓轰粈涔堟枃瀛椾贡鐮侊紵

瀛椾綋鏂囦欢涓嶅寘鍚綘灏濊瘯娓叉煋鐨勫瓧褰€備娇鐢ㄦ敮鎸佷綘瀛楃闆嗙殑瀛椾綋锛堜緥濡?CJK 鐢?NotoSansSC锛夈€?

## Widget

### 濡備綍鍒涘缓鑷畾涔?Widget锛?

缁ф壙 `Widget`锛堟垨 `StatelessWidget`/`StatefulWidget`锛夊苟閲嶅啓鐩稿叧铏氬嚱鏁帮細`Build()`銆乣OnMeasure()`銆乣Paint()` 鍜屼簨浠跺鐞嗗嚱鏁般€傚弬瑙?[Widget 绯荤粺](./widgets)銆?

### `MarkNeedsBuild()` 鍜?`MarkFrameDirty()` 鏈変粈涔堝尯鍒紵

- `MarkNeedsBuild()` 鈥?鏍囪 widget 鍦ㄤ笅涓€甯ч噸寤猴紙瀛愮粍浠跺彲鑳藉彉鍖栵級銆?
- `MarkFrameDirty()` 鈥?瑙﹀彂 layout/paint 寰幆鑰屼笉閲嶅缓 widget銆傜敤浜庣粯鍒舵椂鍙樺寲锛堜緥濡傛嫋鎷藉亸绉汇€佹粴鍔ㄤ綅缃級銆?

### 涓轰粈涔堟垜鐨?Draggable 涓嶈窡闅忓厜鏍囷紵

纭繚娌℃湁鍦?`OnPointerMove()` 涓皟鐢?`MarkNeedsBuild()` 鈥斺€?鎷栨嫿鍋忕Щ浠呭湪缁樺埗鏃跺簲鐢ㄣ€傛鏋跺湪鎸囬拡浜嬩欢鏃惰嚜鍔ㄨ皟鐢?`MarkFrameDirty()`銆?

## 甯冨眬

### 涓轰粈涔堟垜鐨?Widget 涓嶅彲瑙侊紵

- Widget 灏哄涓洪浂锛堟鏌?`OnMeasure()` 杩斿洖鍊硷級銆?
- Widget 鍦ㄨ鍙ｅ锛堟鏌?flex 绾︽潫锛夈€?
- Widget 琚?`ScrollView` 鎴?`ClipRect` 瑁佸壀銆?

### 濡備綍璁?Widget 濉厖鍙敤绌洪棿锛?

浣跨敤 `Expanded`锛堣缃?`flex_grow`锛夋垨鍦?flex 瀹瑰櫒涓缃?`SetWidth(0)` 閰嶅悎 `flex_grow > 0`銆?

## 绾跨▼

### NeoFlux 绾跨▼瀹夊叏鍚楋紵

Widget 鏍戜粎浠庝富绾跨▼璁块棶銆傛覆鏌撳眰鍦ㄧ嫭绔嬬嚎绋嬭繍琛岋紝閫氳繃 SPSC 鐜舰闃熷垪閫氫俊銆備笉瑕佷粠娓叉煋绾跨▼璁块棶 widget銆?

### 鍗忕▼濡備綍宸ヤ綔锛?

NeoFlux 鎻愪緵 `Task<T>` 鍗忕▼绫诲瀷锛屾敮鎸?`co_await Yield()`锛堜笅涓€甯э級鍜?`co_await Sleep(duration)`銆傚崗绋嬬敱 `EventLoop` 鍦ㄤ富绾跨▼璋冨害銆備娇鐢?`std::weak_ptr` 闃叉 `co_await` 鏈熼棿 widget 閿€姣併€?

## 鏁呴殰鎺掓煡

### 搴旂敤鍚姩鏃跺穿婧?

妫€鏌ワ細
1. 瀛椾綋鐩綍瀛樺湪涓斿寘鍚湁鏁堝瓧浣撴枃浠躲€?
2. `Init()` 鍦?`Run()` 涔嬪墠璋冪敤銆?
3. 宸叉敞鍐屽苟 push route銆?
4. 娓叉煋鍚庣鍦ㄤ綘鐨勫钩鍙颁笂鍙楁敮鎸併€?

### clang-tidy 鎶ュ憡璀﹀憡

杩愯 `clang-tidy -p build src/**/*.cpp` 妫€鏌ャ€傞」鐩洰鏍囨槸闆惰鍛娿€傚畬鏁存鏌ユ竻鍗曡 [璐＄尞鎸囧崡](./contributing)銆?

### 濡備綍鍚敤璇︾粏鏃ュ織锛?

```bash
./my_app --verbose_logging=true
```

鏃ュ織榛樿鍐欏叆 `logs/`銆備娇鐢?`--logtostderr=true` 杈撳嚭鍒?stderr銆?
